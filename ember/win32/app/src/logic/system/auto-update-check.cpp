/*
 * ------------------------------------------------------------------------
 * 名称：auto-update-check.cpp
 * 说明：启动时自动检查更新的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/system/auto-update-check.hpp"

namespace ember::win32::logic {

namespace {

/** 自动检查更新设置项的标识。 */
constexpr const char* kAutoCheckSettingId = "update.autoCheck";

}  // namespace

AutoUpdateCheck::AutoUpdateCheck(contracts::RequestBroker& broker, contracts::CoreGateway& core, contracts::AppUpdater& updater, Schedule schedule, unsigned delayMilliseconds)
    : broker_(broker), core_(core), updater_(updater), schedule_(std::move(schedule)), delay_(delayMilliseconds) {}

AutoUpdateCheck::~AutoUpdateCheck() { stop(); }

void AutoUpdateCheck::start() {
  if (started_ || updater_.status().state == "unsupported") return;
  started_ = true;
  stopEvents_ = core_.onEvent([this](const std::string& name, const nlohmann::json& data) {
    if (name != "preferences.changed" || !data.is_object()) return;
    if (data.value("id", std::string()) == kAutoCheckSettingId && data.contains("value")) apply(data["value"]);
  });
  stopStatus_ = core_.onStatusChange([this](const contracts::CoreStatus& status) {
    if (status.state == contracts::CoreState::Running) refresh();
  });
  if (core_.status().state == contracts::CoreState::Running) refresh();
}

void AutoUpdateCheck::stop() {
  *alive_ = false;
  finished_ = true;
  cancel();
  if (stopEvents_) stopEvents_();
  if (stopStatus_) stopStatus_();
  stopEvents_ = nullptr;
  stopStatus_ = nullptr;
}

void AutoUpdateCheck::refresh() {
  const std::weak_ptr<bool> weak = alive_;
  broker_.request("preferences.list", nlohmann::json::object(), [this, weak](contracts::CoreReply reply) {
    const auto alive = weak.lock();
    if (!alive || !*alive || !reply.ok || !reply.result.is_object() || !reply.result.contains("settings") || !reply.result["settings"].is_array()) return;
    for (const auto& setting : reply.result["settings"]) {
      if (setting.is_object() && setting.value("id", std::string()) == kAutoCheckSettingId && setting.contains("value")) {
        apply(setting["value"]);
        return;
      }
    }
  });
}

void AutoUpdateCheck::apply(const nlohmann::json& value) {
  if (finished_) return;
  if (value.is_boolean() && !value.get<bool>()) cancel();
  if (!value.is_boolean() || !value.get<bool>() || timerPending_) return;
  timerPending_ = true;
  const std::weak_ptr<bool> weak = alive_;
  cancelTimer_ = schedule_(delay_, [this, weak] {
    const auto alive = weak.lock();
    if (!alive || !*alive) return;
    timerPending_ = false;
    cancelTimer_ = nullptr;
    finished_ = true;
    static_cast<void>(updater_.check());
  });
}

void AutoUpdateCheck::cancel() {
  if (cancelTimer_) cancelTimer_();
  cancelTimer_ = nullptr;
  timerPending_ = false;
}

}  // namespace ember::win32::logic
