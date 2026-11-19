/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges.cpp
 * 说明：最近打开的卡带清单的规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "shared/recent-cartridges/recent-cartridges.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace ember::win32::shared {

namespace {

constexpr int kFileVersion = 1;
constexpr std::size_t kMaxPathLength = 4096;
constexpr std::string_view kCartridgeExtension = ".nes";

/** 路径合格：非空、不过长、是 .nes 文件（扩展名不分大小写）。 */
bool isValidPath(const std::string& path) {
  if (path.empty() || path.size() > kMaxPathLength || path.size() < kCartridgeExtension.size()) return false;
  return std::equal(kCartridgeExtension.begin(), kCartridgeExtension.end(), path.end() - static_cast<std::ptrdiff_t>(kCartridgeExtension.size()),
                    [](char expected, char actual) { return std::tolower(static_cast<unsigned char>(actual)) == expected; });
}

bool contains(const std::vector<std::string>& list, const std::string& path) { return std::find(list.begin(), list.end(), path) != list.end(); }

}  // namespace

std::vector<std::string> parseRecentCartridges(const nlohmann::json& raw) {
  std::vector<std::string> result;
  if (!raw.is_object() || !raw.contains("items") || !raw["items"].is_array()) return result;
  for (const nlohmann::json& item : raw["items"]) {
    if (item.is_object() && item.contains("path") && item["path"].is_string()) {
      const std::string path = item["path"].get<std::string>();
      if (isValidPath(path) && !contains(result, path)) result.push_back(path);
    }
    if (result.size() >= kMaxRecentCartridges) break;
  }
  return result;
}

std::vector<std::string> addRecentCartridge(const std::vector<std::string>& list, const std::string& path) {
  if (!isValidPath(path)) return list;
  std::vector<std::string> result{path};
  for (const std::string& entry : list) {
    if (entry != path && result.size() < kMaxRecentCartridges) result.push_back(entry);
  }
  return result;
}

std::vector<std::string> removeRecentCartridge(const std::vector<std::string>& list, const std::string& path) {
  if (!contains(list, path)) return list;
  std::vector<std::string> result;
  for (const std::string& entry : list) {
    if (entry != path) result.push_back(entry);
  }
  return result;
}

std::string recentCartridgeName(const std::string& path) {
  const std::size_t separator = path.find_last_of("\\/");
  return separator == std::string::npos ? path : path.substr(separator + 1);
}

nlohmann::json toRecentCartridgesFile(const std::vector<std::string>& list) {
  nlohmann::json items = nlohmann::json::array();
  for (const std::string& path : list) items.push_back({{"path", path}});
  return {{"version", kFileVersion}, {"items", items}};
}

}  // namespace ember::win32::shared
