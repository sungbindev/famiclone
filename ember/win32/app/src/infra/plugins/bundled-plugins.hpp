/*
 * ------------------------------------------------------------------------
 * 名称：bundled-plugins.hpp
 * 说明：预装插件的释放：把资源包里 plugins/ 下的 .emap 插件包解压到用户数据目录下的 bundled-plugins，核心从那里读预装插件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：目录里放一个 .stamp 文件记下资源包的指纹，下次启动指纹相同且文件都在时不重复解压；要重新释放时先解压到旁边的临时目录，写完再换掉旧目录，释放中途出错不会留下半份旧半份新的目录。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <string>

#include "contracts/resources/resource-archive.hpp"

namespace ember::win32::infra {

/** 目录里记下资源包指纹的文件名。 */
inline constexpr const char* kBundledPluginsStamp = ".stamp";

/**
 * 把资源包里的预装插件释放到目录。
 * @param archive 资源包。
 * @param directory 目标目录（用户数据目录下的 bundled-plugins）。
 * @returns 成功（含无须重复释放）时为空文字，失败时是错误说明。
 */
[[nodiscard]] std::string extractBundledPlugins(const contracts::ResourceArchive& archive, const std::filesystem::path& directory);

}  // namespace ember::win32::infra
