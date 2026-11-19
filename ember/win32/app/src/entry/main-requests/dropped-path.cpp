/*
 * ------------------------------------------------------------------------
 * 名称：dropped-path.cpp
 * 说明：按编号取拖入文件路径的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/main-requests/dropped-path.hpp"

#include <algorithm>
#include <cctype>

namespace ember::win32::entry {

std::optional<std::string> resolveDroppedPath(const contracts::DroppedFilesService& files, std::int64_t id, const std::vector<std::string>& extensions) {
  const auto path = files.resolve(id);
  if (!path) return std::nullopt;
  const std::size_t dot = path->find_last_of('.');
  std::string extension = dot == std::string::npos ? std::string{} : path->substr(dot + 1);
  std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return std::find(extensions.begin(), extensions.end(), extension) == extensions.end() ? std::nullopt : path;
}

}  // namespace ember::win32::entry
