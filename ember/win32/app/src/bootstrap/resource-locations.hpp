/*
 * ------------------------------------------------------------------------
 * 名称：resource-locations.hpp
 * 说明：随应用带的资源的位置：预装的 mapper 插件目录、默认 BIOS 目录、解码磁带用的 ffmpeg 程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：正式包里资源在可执行文件旁的 resources 目录（plugins、bios、win32-x64\ffmpeg.exe）；开发时可执行文件在 build\ember\win32\app\<配置>\，
 *       预装插件在 build\ember\plugins，默认 BIOS 在仓库的 ember\bios，ffmpeg 在 ember\core\third_party\ffmpeg\bin\win32-x64；找不到的项为空（核心对缺失的资源按没有处理）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace ember::win32::bootstrap {

/** 资源的位置（UTF-8 路径）。 */
struct ResourceLocations {
  std::optional<std::string> plugins;
  std::optional<std::string> bios;
  std::optional<std::string> ffmpeg;
};

/**
 * 找随应用带的资源。
 * @param executableDirectory 可执行文件所在的目录。
 */
[[nodiscard]] ResourceLocations locateResources(const std::filesystem::path& executableDirectory);

}  // namespace ember::win32::bootstrap
