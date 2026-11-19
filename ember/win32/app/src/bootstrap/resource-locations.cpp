/*
 * ------------------------------------------------------------------------
 * 名称：resource-locations.cpp
 * 说明：随应用带的资源位置的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/resource-locations.hpp"

#include <system_error>
#include <vector>

#include "platform/text/wide-text.hpp"

namespace ember::win32::bootstrap {

namespace {

/** 候选里第一个存在的路径。 */
std::optional<std::string> firstExisting(const std::vector<std::filesystem::path>& candidates, bool directory) {
  for (const std::filesystem::path& candidate : candidates) {
    std::error_code error;
    const bool exists = directory ? std::filesystem::is_directory(candidate, error) : std::filesystem::is_regular_file(candidate, error);
    if (exists) return platform::pathToUtf8(candidate.lexically_normal());
  }
  return std::nullopt;
}

}  // namespace

ResourceLocations locateResources(const std::filesystem::path& executableDirectory) {
  const std::filesystem::path& exe = executableDirectory;
  // 开发时：exe 在 build\ember\win32\app\<配置>，向上三级是 build\ember，向上五级是仓库根目录
  const std::filesystem::path buildRoot = exe / ".." / ".." / "..";
  const std::filesystem::path repositoryRoot = exe / ".." / ".." / ".." / ".." / "..";
  ResourceLocations result;
  result.plugins = firstExisting({exe / "resources" / "plugins", buildRoot / "plugins"}, true);
  result.bios = firstExisting({exe / "resources" / "bios", repositoryRoot / "ember" / "bios"}, true);
  result.ffmpeg = firstExisting({exe / "resources" / "win32-x64" / "ffmpeg.exe", repositoryRoot / "ember" / "core" / "third_party" / "ffmpeg" / "bin" / "win32-x64" / "ffmpeg.exe"}, false);
  return result;
}

}  // namespace ember::win32::bootstrap
