/*
 * ------------------------------------------------------------------------
 * 名称：media-controller.cpp
 * 说明：介质模块控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/media/media-controller.hpp"

namespace ember::win32::logic {

namespace {

constexpr const char* kOpenPrinterDirectory = "打开图片目录";

std::string failedText(const std::string& action, const std::string& reason) { return action + "失败：" + reason; }

std::string messageOf(const nlohmann::json& data) {
  const auto found = data.is_object() ? data.find("message") : data.end();
  return found != data.end() && found->is_string() ? found->get<std::string>() : std::string();
}

}  // namespace

MediaController::MediaController(contracts::RequestBroker& broker, contracts::CoreGateway& core) : broker_(broker), core_(core) {}

MediaController::~MediaController() {
  *alive_ = false;
  for (const auto& stop : stops_) stop();
}

std::function<void()> MediaController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    for (const auto& stop : stops_) stop();
    stops_.clear();
    shell_ = nullptr;
    ++generation_;
    ++floppyRequest_;
    ++printerRequest_;
    ++tapeRequest_;
    ++timelineRequest_;
  };
  stopAll();
  shell_ = &shell;
  stops_.push_back(core_.onEvent([this](const std::string& name, const nlohmann::json& data) { handleEvent(name, data); }));
  stops_.push_back(core_.onStatusChange([this](const contracts::CoreStatus& status) { handleCoreStatus(status); }));
  refreshAll();
  return stopAll;
}

contracts::MediaView MediaController::view() const { return buildMediaView(floppies_, tape_, printer_); }

void MediaController::changed() {
  // 回调里可能取消订阅，先复制一份再逐个调用
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
  if (shell_ != nullptr) shell_->refreshCommands();
}

std::function<void()> MediaController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void MediaController::ask(const std::string& name, nlohmann::json params, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, std::move(params), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    // 控制器已经销毁，或这之间断开过、重新接上过：旧的回复不处理
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

void MediaController::report(const std::string& action, const std::string& reason) {
  if (shell_ != nullptr) shell_->notify(failedText(action, reason), contracts::NotificationLevel::Error);
}

void MediaController::perform(const std::string& action, const std::string& request, nlohmann::json params) {
  if (shell_ == nullptr) return;
  ask(request, std::move(params), [this, action](const contracts::CoreReply& reply) {
    if (!reply.ok) report(action, reply.error.message);
    refreshAll();
  });
}

void MediaController::insertFloppy(int drive) { perform("载入软盘镜像", "media.openFloppy", {{"drive", drive}}); }

void MediaController::insertDroppedFloppy(int drive, std::int64_t droppedId) { perform("载入软盘镜像", "media.openFloppy", {{"drive", drive}, {"droppedId", droppedId}}); }

void MediaController::ejectFloppy(int drive) { perform("取出软盘", "media.floppyEject", {{"drive", drive}}); }

void MediaController::setWriteProtected(int drive, bool writeProtected) { perform("设置写保护", "media.floppyWriteProtect", {{"drive", drive}, {"writeProtected", writeProtected}}); }

void MediaController::ejectTape() { perform("取出磁带", "media.tapeEject", nlohmann::json::object()); }

void MediaController::loadDroppedTape(std::int64_t droppedId) { perform("载入磁带", "media.openTape", {{"droppedId", droppedId}}); }

void MediaController::recordTape() { perform("磁带录音", "media.openTapeRecording", nlohmann::json::object()); }

void MediaController::stopTape() { perform("停止磁带", "media.tapeStop", nlohmann::json::object()); }

void MediaController::seekTape(double seconds) { perform("磁带定位", "media.tapeSeek", {{"position", seconds}}); }

contracts::MediaTimelineView MediaController::timelineView() const { return buildTimelineView(tape_, timeline_); }

void MediaController::playTape() {
  if (shell_ == nullptr) return;
  if (tape_ && tape_->loaded) {
    perform("播放磁带", "media.tapePlay", nlohmann::json::object());
    return;
  }
  // 没有磁带：先选磁带；载入后磁带机已接在通电的主机上就立即开始播放，否则只载入；用户取消时什么都不发生
  ask("media.openTape", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      report("播放磁带", reply.error.message);
      refreshAll();
      return;
    }
    const bool canceled = reply.result.is_object() && reply.result.value("canceled", false);
    const auto loaded = reply.result.is_object() && reply.result.contains("status") ? parseTape(reply.result["status"]) : std::nullopt;
    if (!canceled && loaded && loaded->connected) {
      perform("播放磁带", "media.tapePlay", nlohmann::json::object());
      return;
    }
    refreshAll();
  });
}

void MediaController::handleEvent(const std::string& name, const nlohmann::json& data) {
  if (name == "media.changed" || name == "runtime.stateChanged" || name == "machines.changed") {
    refreshAll();
  } else if (name == "media.printed") {
    refreshPrinter();
    const auto file = data.is_object() ? data.find("fileName") : data.end();
    const auto page = data.is_object() ? data.find("page") : data.end();
    if (file != data.end() && file->is_string() && page != data.end() && page->is_number_integer() && shell_ != nullptr) {
      shell_->notify("打印机输出了第 " + std::to_string(page->get<int>()) + " 页：" + file->get<std::string>(), contracts::NotificationLevel::Info, {kOpenPrinterDirectory, [this] { openPrinterDirectory(); }});
    }
  } else if (name == "media.printFailed") {
    report("打印机输出", messageOf(data));
  } else if (name == "media.tapeAnalyzed") {
    refreshTimeline();
  } else if (name == "media.floppySaveFailed") {
    report("自动保存软盘镜像", messageOf(data));
  }
}

void MediaController::handleCoreStatus(const contracts::CoreStatus& status) {
  if (status.state == contracts::CoreState::Running) {
    refreshAll();
    return;
  }
  clear();
}

void MediaController::openPrinterDirectory() {
  if (shell_ == nullptr) return;
  ask("media.openPrinterDirectory", nlohmann::json::object(), [this](const contracts::CoreReply& reply) {
    if (!reply.ok) shell_->notify(reply.error.message, contracts::NotificationLevel::Error);
  });
}

void MediaController::refreshAll() {
  refreshFloppies();
  refreshPrinter();
  refreshTape();
}

void MediaController::refreshFloppies() {
  if (shell_ == nullptr) return;
  const auto current = ++floppyRequest_;
  ask("media.floppyStatus", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    // 核心没有就绪时读不到，等核心状态推送后再读，不报错
    if (!reply.ok || floppyRequest_ != current) return;
    auto next = parseFloppies(reply.result);
    if (next == floppies_) return;
    floppies_ = std::move(next);
    changed();
  });
}

void MediaController::refreshPrinter() {
  if (shell_ == nullptr) return;
  const auto current = ++printerRequest_;
  ask("media.printerStatus", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (!reply.ok || printerRequest_ != current) return;
    const auto next = parsePrinter(reply.result);
    if (next == printer_) return;
    printer_ = next;
    changed();
  });
}

void MediaController::refreshTape() {
  if (shell_ == nullptr) return;
  const auto current = ++tapeRequest_;
  ask("media.tapeStatus", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (!reply.ok || tapeRequest_ != current) return;
    const auto next = reply.result.is_object() && reply.result.contains("status") ? parseTape(reply.result["status"]) : std::nullopt;
    const bool tapeChanged = next != tape_;
    tape_ = next;
    // 没有磁带时时间轴作废；换了一盘或还在分析时重新读（分析中时磁带状态可能一直没变，进度要靠这里推进）
    if (!tape_ || !tape_->loaded) timeline_.reset();
    if (timelineNeedsRefresh(tape_, timeline_)) refreshTimeline();
    if (tapeChanged) changed();
  });
}

void MediaController::refreshTimeline() {
  if (shell_ == nullptr) return;
  const auto current = ++timelineRequest_;
  ask("media.tapeTimeline", nlohmann::json::object(), [this, current](const contracts::CoreReply& reply) {
    if (!reply.ok || timelineRequest_ != current) return;
    auto next = parseTimeline(reply.result);
    if (next == timeline_) return;
    timeline_ = std::move(next);
    changed();
  });
}

void MediaController::clear() {
  ++floppyRequest_;
  ++printerRequest_;
  ++tapeRequest_;
  ++timelineRequest_;
  floppies_.clear();
  printer_.reset();
  tape_.reset();
  timeline_.reset();
  changed();
}

}  // namespace ember::win32::logic
