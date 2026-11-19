/*
 * ------------------------------------------------------------------------
 * 名称：last-directories.cpp
 * 说明：记住上次目录的规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/last-directories/last-directories.hpp"

#include <algorithm>

namespace ember::win32::shared {

namespace {

constexpr int kFileVersion = 1;
constexpr std::size_t kMaxPurposeLength = 64;
constexpr std::size_t kMaxDirectoryLength = 1024;

bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

}  // namespace

bool isValidDirectoryPurpose(const std::string& purpose) {
  if (purpose.empty() || purpose.size() > kMaxPurposeLength || !isAlpha(purpose[0])) return false;
  return std::all_of(purpose.begin(), purpose.end(), [](char c) { return isAlpha(c) || (c >= '0' && c <= '9'); });
}

bool isValidRememberedDirectory(const std::string& directory) { return !directory.empty() && directory.size() <= kMaxDirectoryLength; }

std::map<std::string, std::string> parseLastDirectories(const nlohmann::json& raw) {
  std::map<std::string, std::string> result;
  if (!raw.is_object() || !raw.contains("version") || raw["version"] != kFileVersion || !raw.contains("directories") || !raw["directories"].is_object()) return result;
  for (const auto& [purpose, directory] : raw["directories"].items()) {
    if (result.size() >= kMaxLastDirectoryPurposes) break;
    if (isValidDirectoryPurpose(purpose) && directory.is_string() && isValidRememberedDirectory(directory.get<std::string>())) result[purpose] = directory.get<std::string>();
  }
  return result;
}

nlohmann::json toLastDirectoriesFile(const std::map<std::string, std::string>& directories) {
  nlohmann::json items = nlohmann::json::object();
  for (const auto& [purpose, directory] : directories) items[purpose] = directory;
  return {{"version", kFileVersion}, {"directories", items}};
}

}  // namespace ember::win32::shared
