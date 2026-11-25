/*
 * ------------------------------------------------------------------------
 * 名称：directory-opener.hpp
 * 说明：用系统的文件管理器（资源管理器）打开目录：目录不存在时先创建。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：路径是 UTF-8；失败时返回错误说明（不抛异常），成功返回空文字。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <string>

namespace ember::win32::platform {

/**
 * 用系统的文件管理器打开目录。
 * @param directory 目录；不存在时先创建。
 * @returns 成功时为空文字，否则是错误说明。
 */
[[nodiscard]] std::string openDirectoryInFileManager(const std::filesystem::path& directory);

/**
 * 在系统的文件管理器里显示一个文件（打开它所在的目录并选中它）。
 * @param file 文件的完整路径。
 * @returns 成功时为空文字，否则是错误说明。
 */
[[nodiscard]] std::string revealFileInFileManager(const std::filesystem::path& file);

/**
 * 用系统默认的程序打开一个文件（如用图片查看程序打开 PNG）。
 * @param file 文件的完整路径。
 * @returns 成功时为空文字，否则是错误说明。
 */
[[nodiscard]] std::string openFileWithDefaultProgram(const std::filesystem::path& file);

}  // namespace ember::win32::platform
