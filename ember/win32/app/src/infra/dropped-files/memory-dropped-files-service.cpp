/*
 * ------------------------------------------------------------------------
 * 名称：memory-dropped-files-service.cpp
 * 说明：拖入文件服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/dropped-files/memory-dropped-files-service.hpp"

#include <algorithm>

namespace ember::win32::infra {

namespace {

constexpr std::size_t kMaxKept = 64;

/** 路径里的文件名。 */
std::string fileName(const std::string& path) {
  const std::size_t separator = path.find_last_of("\\/");
  return separator == std::string::npos ? path : path.substr(separator + 1);
}

}  // namespace

std::vector<contracts::DroppedFile> MemoryDroppedFilesService::registerFiles(const std::vector<std::string>& paths) {
  std::vector<contracts::DroppedFile> result;
  for (const std::string& path : paths) {
    const std::int64_t id = nextId_++;
    files_.emplace_back(id, path);
    if (files_.size() > kMaxKept) files_.pop_front();
    result.push_back({id, fileName(path)});
  }
  return result;
}

std::optional<std::string> MemoryDroppedFilesService::resolve(std::int64_t id) const {
  const auto found = std::find_if(files_.begin(), files_.end(), [id](const auto& entry) { return entry.first == id; });
  return found == files_.end() ? std::nullopt : std::optional<std::string>(found->second);
}

}  // namespace ember::win32::infra
