/*
 * ------------------------------------------------------------------------
 * 名称：file-recent-cartridges-service.cpp
 * 说明：最近打开的卡带服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/recent-cartridges/file-recent-cartridges-service.hpp"

#include "shared/recent-cartridges/recent-cartridges.hpp"

namespace ember::win32::infra {

FileRecentCartridgesService::FileRecentCartridgesService(platform::JsonFileStore& store) : store_(store) {
  if (const auto saved = store_.read()) paths_ = shared::parseRecentCartridges(*saved);
  for (const std::string& path : paths_) idOf(path);
}

std::vector<contracts::RecentCartridge> FileRecentCartridgesService::list() const {
  std::vector<contracts::RecentCartridge> result;
  for (const std::string& path : paths_) result.push_back({idOf(path), shared::recentCartridgeName(path)});
  return result;
}

void FileRecentCartridgesService::add(const std::string& path) { replace(shared::addRecentCartridge(paths_, path)); }

void FileRecentCartridgesService::remove(const std::string& path) { replace(shared::removeRecentCartridge(paths_, path)); }

std::optional<std::string> FileRecentCartridgesService::resolve(std::int64_t id) const {
  for (const std::string& path : paths_) {
    if (idOf(path) == id) return path;
  }
  return std::nullopt;
}

std::int64_t FileRecentCartridgesService::idOf(const std::string& path) const {
  const auto found = ids_.find(path);
  if (found != ids_.end()) return found->second;
  const std::int64_t id = nextId_++;
  ids_[path] = id;
  return id;
}

void FileRecentCartridgesService::replace(std::vector<std::string> next) {
  if (next == paths_) return;
  paths_ = std::move(next);
  store_.write(shared::toRecentCartridgesFile(paths_));
}

}  // namespace ember::win32::infra
