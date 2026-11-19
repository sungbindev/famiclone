/*
 * ------------------------------------------------------------------------
 * 名称：file-last-directories-service.cpp
 * 说明：记住上次目录服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/last-directories/file-last-directories-service.hpp"

#include "shared/last-directories/last-directories.hpp"

namespace ember::win32::infra {

FileLastDirectoriesService::FileLastDirectoriesService(platform::JsonFileStore& store) : store_(store) {
  if (const auto saved = store_.read()) directories_ = shared::parseLastDirectories(*saved);
}

std::optional<std::string> FileLastDirectoriesService::get(const std::string& purpose) const {
  const auto found = directories_.find(purpose);
  return found == directories_.end() ? std::nullopt : std::optional<std::string>(found->second);
}

void FileLastDirectoriesService::set(const std::string& purpose, const std::string& directory) {
  if (!shared::isValidDirectoryPurpose(purpose) || !shared::isValidRememberedDirectory(directory)) return;
  const auto found = directories_.find(purpose);
  if (found != directories_.end() && found->second == directory) return;
  if (found == directories_.end() && directories_.size() >= shared::kMaxLastDirectoryPurposes) return;
  directories_[purpose] = directory;
  store_.write(shared::toLastDirectoriesFile(directories_));
}

}  // namespace ember::win32::infra
