/*
 * ------------------------------------------------------------------------
 * 名称：theme-sync.cpp
 * 说明：主题同步的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/preferences/theme-sync.hpp"

#include <memory>
#include <optional>

namespace ember::win32::entry {

namespace {

constexpr const char* kThemeSettingId = "appearance.theme";

std::optional<ThemeSource> parseSource(const nlohmann::json& value) {
  if (!value.is_string()) return std::nullopt;
  const std::string text = value.get<std::string>();
  if (text == "system") return ThemeSource::System;
  if (text == "dark") return ThemeSource::Dark;
  if (text == "light") return ThemeSource::Light;
  return std::nullopt;
}

}  // namespace

ThemeSync::ThemeSync(contracts::CoreGateway& core, std::function<void(ThemeSource)> apply, std::function<void(const std::string&)> onError, std::function<void()> onFirst)
    : core_(core), apply_(std::move(apply)), onError_(std::move(onError)), onFirst_(std::move(onFirst)), alive_(std::make_shared<bool>(true)) {
  stopEvents_ = core_.onEvent([this](const std::string& name, const nlohmann::json& data) {
    if (name != "preferences.changed" || !data.is_object() || data.value("id", std::string{}) != kThemeSettingId || !data.contains("value")) return;
    if (const auto source = parseSource(data["value"])) apply_(*source);
  });
  stopStatus_ = core_.onStatusChange([this](const contracts::CoreStatus& status) {
    if (status.state == contracts::CoreState::Running) refresh();
    // 核心起不来时不会再有主题，不能让主窗口一直等
    if (status.state == contracts::CoreState::Failed) settle();
  });
  if (core_.status().state == contracts::CoreState::Running) refresh();
}

ThemeSync::~ThemeSync() {
  *alive_ = false;
  stopEvents_();
  stopStatus_();
}

void ThemeSync::settle() {
  if (settled_) return;
  settled_ = true;
  if (onFirst_) onFirst_();
}

void ThemeSync::refresh() {
  core_.request("preferences.list", nlohmann::json::object(), [this, alive = alive_](contracts::CoreReply reply) {
    if (!*alive) return;
    if (!reply.ok) {
      if (onError_) onError_("读取主题设置失败：" + reply.error.code);
    } else if (reply.result.is_object() && reply.result.contains("settings") && reply.result["settings"].is_array()) {
      for (const nlohmann::json& setting : reply.result["settings"]) {
        if (setting.is_object() && setting.value("id", std::string{}) == kThemeSettingId && setting.contains("value")) {
          if (const auto source = parseSource(setting["value"])) apply_(*source);
          break;
        }
      }
    }
    settle();
  });
}

}  // namespace ember::win32::entry
