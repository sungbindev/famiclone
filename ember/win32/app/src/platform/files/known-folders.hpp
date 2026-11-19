/*
 * ------------------------------------------------------------------------
 * 名称：known-folders.hpp
 * 说明：系统与程序自己的目录：漫游应用数据目录、默认的用户数据目录、可执行文件所在的目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：没有 ember-data 目录时，默认的用户数据目录是“<漫游应用数据>\Ember”。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>

namespace ember::win32::platform {

/** 当前用户的漫游应用数据目录（%APPDATA%）；取不到时为空路径。 */
[[nodiscard]] std::filesystem::path roamingAppDataDirectory();

/** 默认的用户数据目录：<漫游应用数据>\Ember；取不到漫游应用数据目录时为空路径。 */
[[nodiscard]] std::filesystem::path defaultUserDataDirectory();

/** 当前可执行文件的完整路径。 */
[[nodiscard]] std::filesystem::path executablePath();

/** 当前可执行文件所在的目录。 */
[[nodiscard]] std::filesystem::path executableDirectory();

}  // namespace ember::win32::platform
