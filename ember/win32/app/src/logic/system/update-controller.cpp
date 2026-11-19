/*
 * ------------------------------------------------------------------------
 * 名称：update-controller.cpp
 * 说明：应用升级控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/system/update-controller.hpp"

namespace ember::win32::logic {

namespace {

/** 用户可见的文字。 */
constexpr const char* kCheck = "检查更新";
constexpr const char* kChecking = "正在检查更新…";
constexpr const char* kDownloading = "正在下载更新…";
constexpr const char* kInstall = "重启并安装更新";
constexpr const char* kCheckStarted = "正在检查更新…";
constexpr const char* kUnsupported = "当前运行方式不支持检查更新。";
constexpr const char* kFound = "发现新版本，正在下载。";
constexpr const char* kUpToDate = "已是最新版本。";
constexpr const char* kCheckFailed = "检查更新失败";
constexpr const char* kInstallFailed = "无法重启安装更新";

/** 请求回复里的 status 转成升级状态；格式不对时没有值。 */
std::optional<contracts::UpdateStatus> statusOf(const contracts::CoreReply& reply) {
  if (!reply.ok || !reply.result.is_object()) return std::nullopt;
  const auto status = reply.result.find("status");
  if (status == reply.result.end() || !status->is_object() || !status->contains("state") || !(*status)["state"].is_string()) return std::nullopt;
  contracts::UpdateStatus result;
  result.state = (*status)["state"].get<std::string>();
  if (status->contains("version") && (*status)["version"].is_string()) result.version = (*status)["version"].get<std::string>();
  if (status->contains("message") && (*status)["message"].is_string()) result.message = (*status)["message"].get<std::string>();
  return result;
}

std::string errorText(const contracts::CoreReply& reply, const char* fallback) { return reply.error.message.empty() ? std::string(fallback) : reply.error.message; }

}  // namespace

UpdateController::UpdateController(contracts::RequestBroker& broker, contracts::AppUpdater& updater) : broker_(broker), updater_(updater) {}

UpdateController::~UpdateController() {
  *alive_ = false;
  if (stop_) stop_();
}

std::function<void()> UpdateController::attach(contracts::ShellApi& shell) {
  const auto stopAll = [this] {
    if (stop_) stop_();
    stop_ = nullptr;
    shell_ = nullptr;
    ++generation_;
  };
  stopAll();
  shell_ = &shell;
  stop_ = updater_.subscribe([this](const contracts::UpdateStatus& status) { apply(status, true); });
  ask("system.updateStatus", [this](const contracts::CoreReply& reply) {
    if (const auto status = statusOf(reply)) apply(*status, false);
  });
  return stopAll;
}

void UpdateController::ask(const std::string& name, std::function<void(const contracts::CoreReply&)> done) {
  const auto alive = alive_;
  const auto generation = generation_;
  broker_.request(name, nlohmann::json::object(), [this, alive, generation, done = std::move(done)](contracts::CoreReply reply) {
    if (!*alive || generation != generation_ || shell_ == nullptr) return;
    done(reply);
  });
}

std::string UpdateController::title() const {
  if (status_.state == "checking") return kChecking;
  if (status_.state == "downloading") return kDownloading;
  if (status_.state == "ready") return status_.version ? std::string(kInstall) + "（" + *status_.version + "）" : std::string(kInstall);
  return kCheck;
}

bool UpdateController::canRun() const { return status_.state != "checking" && status_.state != "downloading"; }

void UpdateController::run(contracts::ShellApi& shell) {
  if (shell_ == nullptr || !canRun()) return;
  if (status_.state == "ready") {
    ask("system.installUpdate", [this](const contracts::CoreReply& reply) {
      if (!reply.ok && shell_ != nullptr) shell_->notify(errorText(reply, kInstallFailed), contracts::NotificationLevel::Error);
    });
    return;
  }
  manual_ = true;
  ask("system.checkUpdate", [this](const contracts::CoreReply& reply) {
    if (!reply.ok) {
      manual_ = false;
      shell_->notify(errorText(reply, kCheckFailed), contracts::NotificationLevel::Error);
      return;
    }
    // 完成、失败等结果由状态推送告知；这里只处理请求返回时就已经确定的两种情况
    const auto status = statusOf(reply);
    if (status && status->state == "unsupported") {
      manual_ = false;
      shell_->notify(kUnsupported, contracts::NotificationLevel::Info);
    } else if (status && status->state == "checking") {
      shell_->notify(kCheckStarted, contracts::NotificationLevel::Info);
    }
  });
  static_cast<void>(shell);
}

void UpdateController::apply(const contracts::UpdateStatus& next, bool announce) {
  status_ = next;
  if (shell_ == nullptr) return;
  shell_->refreshCommands();
  if (!announce) return;
  if (next.state == "ready") {
    manual_ = false;
    shell_->notify((next.version ? "新版本 " + *next.version + " " : std::string("新版本")) + "已下载好，在“帮助”菜单里选择“重启并安装更新”即可使用。", contracts::NotificationLevel::Info);
  } else if (next.state == "downloading") {
    if (manual_) shell_->notify(kFound, contracts::NotificationLevel::Info);
  } else if (next.state == "upToDate") {
    if (manual_) shell_->notify(kUpToDate, contracts::NotificationLevel::Info);
    manual_ = false;
  } else if (next.state == "error") {
    if (manual_) shell_->notify(next.message ? std::string(kCheckFailed) + "：" + *next.message : std::string(kCheckFailed), contracts::NotificationLevel::Error);
    manual_ = false;
  }
}

}  // namespace ember::win32::logic
