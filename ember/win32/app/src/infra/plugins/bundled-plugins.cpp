/*
 * ------------------------------------------------------------------------
 * 名称：bundled-plugins.cpp
 * 说明：预装插件的释放的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/plugins/bundled-plugins.hpp"

#include <fstream>
#include <system_error>
#include <vector>

#include "platform/text/wide-text.hpp"

namespace ember::win32::infra {

namespace {

namespace fs = std::filesystem;

constexpr const char* kPluginsPrefix = "plugins/";
constexpr const char* kPluginExtension = ".emap";

/** 资源包里预装插件条目的文件名（不含 plugins/ 前缀）：只认 plugins/ 下直接放的 .emap，文件名里不能有路径分隔符、冒号与上级目录。 */
std::vector<std::string> pluginFileNames(const contracts::ResourceArchive& archive) {
  std::vector<std::string> result;
  for (const std::string& name : archive.names()) {
    if (name.rfind(kPluginsPrefix, 0) != 0) continue;
    const std::string file = name.substr(std::string(kPluginsPrefix).size());
    if (file.size() <= std::string(kPluginExtension).size() || file.compare(file.size() - 5, 5, kPluginExtension) != 0) continue;
    if (file.find_first_of("/\\:") != std::string::npos || file.find("..") != std::string::npos) continue;
    result.push_back(file);
  }
  return result;
}

std::string readStamp(const fs::path& file) {
  std::ifstream stream(file, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

}  // namespace

std::string extractBundledPlugins(const contracts::ResourceArchive& archive, const fs::path& directory) {
  const std::vector<std::string> files = pluginFileNames(archive);
  const std::string fingerprint = archive.fingerprint();
  std::error_code error;

  // 指纹相同且文件都在：不重复释放
  if (readStamp(directory / kBundledPluginsStamp) == fingerprint) {
    bool complete = true;
    for (const std::string& file : files) {
      if (!fs::is_regular_file(directory / platform::pathFromUtf8(file), error)) {
        complete = false;
        break;
      }
    }
    if (complete) return {};
  }

  const fs::path staging = fs::path(directory.wstring() + L".tmp");
  fs::remove_all(staging, error);
  fs::create_directories(staging, error);
  if (error) return "无法创建预装插件的目录：" + error.message();
  for (const std::string& file : files) {
    const auto content = archive.read(std::string(kPluginsPrefix) + file);
    if (!content) {
      fs::remove_all(staging, error);
      return "读不出资源包里的预装插件：" + file;
    }
    std::ofstream stream(staging / platform::pathFromUtf8(file), std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(content->data()), static_cast<std::streamsize>(content->size()));
    stream.close();
    if (!stream) {
      fs::remove_all(staging, error);
      return "无法写出预装插件：" + file;
    }
  }
  {
    std::ofstream stamp(staging / kBundledPluginsStamp, std::ios::binary | std::ios::trunc);
    stamp << fingerprint;
    if (!stamp) {
      fs::remove_all(staging, error);
      return "无法写出预装插件的标记文件";
    }
  }
  fs::remove_all(directory, error);
  error.clear();
  fs::rename(staging, directory, error);
  if (error) {
    const std::string reason = error.message();
    fs::remove_all(staging, error);
    return "无法换上新的预装插件目录：" + reason;
  }
  return {};
}

}  // namespace ember::win32::infra
