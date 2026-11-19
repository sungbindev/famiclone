/*
 * ------------------------------------------------------------------------
 * 名称：known-folders.hpp
 * 说明：系统与程序自己的目录：漫游应用数据目录、默认的用户数据目录、可执行文件所在的目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：默认的用户数据目录与 Electron 外壳相同，是“<漫游应用数据>\Ember”（Electron 用产品名 Ember 作目录名），两个外壳共用同一份数据。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>

namespace ember::win32::platform {

/** 当前用户的漫游应用数据目录（%APPDATA%）；取不到时为空路径。 */
[[nodiscard]] std::filesystem::path roamingAppDataDirectory();

/** 默认的用户数据目录：<漫游应用数据>\Ember；取不到漫游应用数据目录时为空路径。 */
[[nodiscard]] std::filesystem::path defaultUserDataDirectory();

/** 当前可执行文件所在的目录。 */
[[nodiscard]] std::filesystem::path executableDirectory();

}  // namespace ember::win32::platform
