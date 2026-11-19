/*
 * ------------------------------------------------------------------------
 * 名称：allow-multiple-instances.cpp
 * 说明：读取“允许同时运行多个实例”的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/preferences/allow-multiple-instances.hpp"

namespace ember::win32::shared {

namespace {

constexpr const char* kSettingId = "app.allowMultipleInstances";
constexpr int kFileVersion = 1;

}  // namespace

bool readAllowMultipleInstances(const nlohmann::json& preferences) {
  if (!preferences.is_object() || !preferences.contains("version") || preferences["version"] != kFileVersion) return false;
  if (!preferences.contains("values") || !preferences["values"].is_object()) return false;
  const nlohmann::json& values = preferences["values"];
  return values.contains(kSettingId) && values[kSettingId].is_boolean() && values[kSettingId].get<bool>();
}

}  // namespace ember::win32::shared
