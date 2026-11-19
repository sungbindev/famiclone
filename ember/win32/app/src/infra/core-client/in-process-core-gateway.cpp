/*
 * ------------------------------------------------------------------------
 * 名称：in-process-core-gateway.cpp
 * 说明：进程内核心网关的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：Shared 里的内容只在界面线程读写；核心线程发来的回调先投递到界面线程，再经 weak_ptr 确认网关还在。
 * ------------------------------------------------------------------------
 */

#include "infra/core-client/in-process-core-gateway.hpp"

#include <exception>
#include <map>
#include <utility>

#include "bootstrap/core.hpp"
#include "platform/streams/frame-output.hpp"

namespace ember::win32::infra {

namespace {

/** 核心的通用错误码，与 core/src/shared/errors/core-error.hpp 一致。 */
constexpr const char* kUnavailableCode = "core.unavailable";
constexpr const char* kInternalCode = "core.internal";

constexpr const char* kUnavailableMessage = "核心尚未就绪或已停止";
constexpr const char* kInternalMessage = "请求失败，详情已写入日志";

contracts::CoreReply unavailableReply() { return contracts::CoreReply::failure(kUnavailableCode, kUnavailableMessage); }

/** 核心的回复文字转成回复：成功时是结果 JSON，失败时是 {"code","message"}。 */
contracts::CoreReply parseReply(bool ok, const std::string& payload) {
  nlohmann::json json = nlohmann::json::parse(payload, nullptr, false);
  if (ok) return contracts::CoreReply::success(json.is_discarded() ? nlohmann::json::object() : std::move(json));
  if (json.is_object() && json.contains("code") && json["code"].is_string()) {
    return contracts::CoreReply::failure(json["code"].get<std::string>(), json.contains("message") && json["message"].is_string() ? json["message"].get<std::string>() : std::string{});
  }
  return contracts::CoreReply::failure(kInternalCode, kInternalMessage);
}

}  // namespace

struct InProcessCoreGateway::Shared {
  contracts::CoreStatus status;
  bool stopping{false};
  int nextHandle{1};
  std::map<int, contracts::CoreEventHandler> eventHandlers;
  std::map<int, contracts::CoreStatusHandler> statusHandlers;

  /** 核心就绪之前排队的请求。 */
  struct Pending {
    std::string name;
    nlohmann::json params;
    contracts::CoreReplyHandler done;
  };
  std::vector<Pending> pending;
};

InProcessCoreGateway::InProcessCoreGateway(InProcessCoreOptions options, platform::UiThread& ui, LatestFrameSlot& frames, contracts::ErrorLogger& logger)
    : options_(std::move(options)), ui_(ui), frames_(frames), logger_(logger), shared_(std::make_shared<Shared>()), core_(std::make_unique<ember::bootstrap::Core>()) {}

InProcessCoreGateway::~InProcessCoreGateway() { stop(); }

void InProcessCoreGateway::setStatus(contracts::CoreStatus status) {
  shared_->status = std::move(status);
  // 处理函数里可能取消订阅，先复制一份再依次调用
  std::vector<contracts::CoreStatusHandler> handlers;
  for (const auto& [handle, handler] : shared_->statusHandlers) handlers.push_back(handler);
  for (const auto& handler : handlers) handler(shared_->status);
}

void InProcessCoreGateway::start() {
  if (shared_->status.state != contracts::CoreState::Stopped || shared_->stopping) return;
  setStatus(contracts::CoreStatus{contracts::CoreState::Starting, {}, {}, {}});

  const std::weak_ptr<Shared> weak = shared_;
  platform::UiThread* const ui = &ui_;
  core_->setEventHandler([weak, ui](const std::string& name, const std::string& dataJson) {
    ui->post([weak, name, dataJson] {
      const auto shared = weak.lock();
      if (!shared) return;
      nlohmann::json data = nlohmann::json::parse(dataJson, nullptr, false);
      if (data.is_discarded()) data = nullptr;
      std::vector<contracts::CoreEventHandler> handlers;
      for (const auto& [handle, handler] : shared->eventHandlers) handlers.push_back(handler);
      for (const auto& handler : handlers) handler(name, data);
    });
  });
  LatestFrameSlot* const frames = &frames_;
  core_->setFrameHandler([frames](const ember::platform::FrameView& frame) { frames->publish(frame.width, frame.height, frame.number, frame.pixels.data()); });

  nlohmann::json options{{"userDataPath", options_.userDataPath}, {"appVersion", options_.appVersion}};
  if (options_.bundledPluginsPath) options["bundledPluginsPath"] = *options_.bundledPluginsPath;
  if (options_.bundledBiosPath) options["bundledBiosPath"] = *options_.bundledBiosPath;
  if (options_.ffmpegPath) options["ffmpegPath"] = *options_.ffmpegPath;
  if (options_.debugPort) options["debugPort"] = *options_.debugPort;
  if (options_.debugToken) options["debugToken"] = *options_.debugToken;

  starter_ = std::thread([this, weak, options = options.dump()] {
    bool ok = true;
    std::string payload;
    try {
      payload = core_->start(options);
    } catch (const std::exception& error) {
      ok = false;
      payload = error.what();
    }
    ui_.post([this, weak, ok, payload] {
      const auto shared = weak.lock();
      if (shared) finishStart(ok, payload);
    });
  });
}

void InProcessCoreGateway::finishStart(bool ok, const std::string& payload) {
  if (shared_->stopping) return;
  if (!ok) {
    logger_.write({std::nullopt, kInternalCode, "核心启动失败：" + payload, nlohmann::json::object()});
    contracts::CoreStatus failed{contracts::CoreState::Failed, {}, {}, payload};
    auto pending = std::move(shared_->pending);
    shared_->pending.clear();
    setStatus(std::move(failed));
    for (auto& request : pending) request.done(unavailableReply());
    return;
  }
  const nlohmann::json started = nlohmann::json::parse(payload, nullptr, false);
  contracts::CoreStatus running{contracts::CoreState::Running, {}, {}, {}};
  if (started.is_object()) {
    if (started.contains("coreVersion") && started["coreVersion"].is_string()) running.coreVersion = started["coreVersion"].get<std::string>();
    if (started.contains("requests") && started["requests"].is_array()) {
      for (const nlohmann::json& name : started["requests"]) {
        if (name.is_string()) running.requests.push_back(name.get<std::string>());
      }
    }
  }
  setStatus(std::move(running));
  auto pending = std::move(shared_->pending);
  shared_->pending.clear();
  for (auto& request : pending) dispatch(request.name, std::move(request.params), std::move(request.done));
}

void InProcessCoreGateway::dispatch(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) {
  const std::string id = "w-" + std::to_string(nextRequestId_.fetch_add(1));
  const std::weak_ptr<Shared> weak = shared_;
  platform::UiThread* const ui = &ui_;
  core_->request(id, name, params.dump(), [weak, ui, done = std::move(done)](bool ok, std::string payload) {
    ui->post([weak, done, ok, payload = std::move(payload)] {
      // 网关已销毁时请求没有人关心结果
      if (weak.lock()) done(parseReply(ok, payload));
    });
  });
}

void InProcessCoreGateway::request(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) {
  switch (shared_->status.state) {
    case contracts::CoreState::Starting:
      shared_->pending.push_back({name, std::move(params), std::move(done)});
      return;
    case contracts::CoreState::Running:
      if (!shared_->stopping) {
        dispatch(name, std::move(params), std::move(done));
        return;
      }
      break;
    default:
      break;
  }
  ui_.post([weak = std::weak_ptr<Shared>(shared_), done = std::move(done)] {
    if (weak.lock()) done(unavailableReply());
  });
}

void InProcessCoreGateway::sendInput(const nlohmann::json& message) {
  if (shared_->status.state != contracts::CoreState::Running || shared_->stopping) return;
  try {
    core_->sendInput(message.dump());
  } catch (const std::exception&) {
    // 核心正在停止或消息不合格时丢弃
  }
}

void InProcessCoreGateway::stop() {
  if (shared_->stopping) return;
  shared_->stopping = true;
  // 启动线程还在读插件时先等它结束，再停止核心
  if (starter_.joinable()) starter_.join();
  const bool started = shared_->status.state == contracts::CoreState::Running || shared_->status.state == contracts::CoreState::Starting;
  if (started) core_->stop();
  auto pending = std::move(shared_->pending);
  shared_->pending.clear();
  for (auto& request : pending) request.done(unavailableReply());
  if (shared_->status.state != contracts::CoreState::Stopped) setStatus(contracts::CoreStatus{contracts::CoreState::Stopped, {}, {}, {}});
}

contracts::CoreStatus InProcessCoreGateway::status() const { return shared_->status; }

std::function<void()> InProcessCoreGateway::onEvent(contracts::CoreEventHandler handler) {
  const int handle = shared_->nextHandle++;
  shared_->eventHandlers[handle] = std::move(handler);
  return [weak = std::weak_ptr<Shared>(shared_), handle] {
    if (const auto shared = weak.lock()) shared->eventHandlers.erase(handle);
  };
}

std::function<void()> InProcessCoreGateway::onStatusChange(contracts::CoreStatusHandler handler) {
  const int handle = shared_->nextHandle++;
  shared_->statusHandlers[handle] = std::move(handler);
  return [weak = std::weak_ptr<Shared>(shared_), handle] {
    if (const auto shared = weak.lock()) shared->statusHandlers.erase(handle);
  };
}

}  // namespace ember::win32::infra
