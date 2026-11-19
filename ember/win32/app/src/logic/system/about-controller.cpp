/*
 * ------------------------------------------------------------------------
 * 名称：about-controller.cpp
 * 说明：“关于 Ember”对话框控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/system/about-controller.hpp"

namespace ember::win32::logic {

namespace {

/** 对话框里的固定文字。 */
constexpr const char* kAppName = "Ember";
constexpr const char* kAuthor = "sungbin";
constexpr const char* kEmail = "sungbin@3578.cn";
constexpr const char* kQq = "46427991";
constexpr const char* kProjectUrl = "https://github.com/sungbindev/famiclone/tree/master/ember";
constexpr const char* kLicense = "GNU 通用公共许可证第 3 版（GPL-3.0）";

}  // namespace

AboutController::AboutController(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::ExternalLinks& links, std::vector<contracts::ThirdPartyNotice> notices, std::string buildTime)
    : broker_(broker), core_(core), links_(links), notices_(std::move(notices)), buildTime_(std::move(buildTime)) {}

AboutController::~AboutController() {
  *alive_ = false;
  if (stopStatus_) stopStatus_();
}

std::function<void()> AboutController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    if (stopStatus_) stopStatus_();
    stopStatus_ = nullptr;
    shell_ = nullptr;
    ++generation_;
    ++loadRequest_;
  };
  stopAll();
  shell_ = &shell;
  stopStatus_ = core_.onStatusChange([this](const contracts::CoreStatus& status) {
    if (status.state == contracts::CoreState::Running) {
      load();
      return;
    }
    ++loadRequest_;
    load_ = contracts::AboutVersionLoad::Loading;
    changed();
  });
  load();
  return stopAll;
}

contracts::AboutView AboutController::view() const {
  contracts::AboutView result;
  result.appName = kAppName;
  result.versionLoad = load_;
  result.appVersion = appVersion_;
  result.coreVersion = coreVersion_;
  result.buildTime = buildTime_;
  result.author = kAuthor;
  result.email = kEmail;
  result.qq = kQq;
  result.projectUrl = kProjectUrl;
  result.license = kLicense;
  result.notices = notices_;
  return result;
}

void AboutController::load() {
  if (shell_ == nullptr) return;
  const auto current = ++loadRequest_;
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request("system.version", nlohmann::json::object(), [this, alive, generation, current](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || loadRequest_ != current) return;
    const bool ok = reply.ok && reply.result.is_object() && reply.result.value("appVersion", nlohmann::json(nullptr)).is_string() && reply.result.value("coreVersion", nlohmann::json(nullptr)).is_string();
    if (ok) {
      load_ = contracts::AboutVersionLoad::Ready;
      appVersion_ = reply.result["appVersion"].get<std::string>();
      coreVersion_ = reply.result["coreVersion"].get<std::string>();
    } else {
      load_ = contracts::AboutVersionLoad::Failed;
      appVersion_.clear();
      coreVersion_.clear();
    }
    changed();
  });
}

void AboutController::changed() {
  const auto listeners = listeners_;
  for (const auto& [id, listener] : listeners) {
    if (listeners_.find(id) != listeners_.end()) listener();
  }
}

std::function<void()> AboutController::subscribe(std::function<void()> handler) {
  const int id = nextListener_++;
  listeners_[id] = std::move(handler);
  const auto alive = alive_;
  return [this, alive, id] {
    if (*alive) listeners_.erase(id);
  };
}

void AboutController::openProject() {
  if (shell_ == nullptr) return;
  const std::string error = links_.open(kProjectUrl);
  if (!error.empty()) shell_->notify("无法打开项目地址：" + error, contracts::NotificationLevel::Error);
}

}  // namespace ember::win32::logic
