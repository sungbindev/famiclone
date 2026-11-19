/*
 * ------------------------------------------------------------------------
 * 名称：wide-text.hpp
 * 说明：UTF-8 与 UTF-16 文字的互相转换：应用内部的文字与路径用 UTF-8，调用 Windows 接口时转成 UTF-16。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：转换失败（非法的编码）时得到空文字。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ember::win32::platform {

/** UTF-8 转 UTF-16。 */
[[nodiscard]] std::wstring widen(std::string_view utf8);

/** UTF-16 转 UTF-8。 */
[[nodiscard]] std::string narrow(std::wstring_view utf16);

/** 路径转 UTF-8 文字。 */
[[nodiscard]] std::string pathToUtf8(const std::filesystem::path& path);

/** UTF-8 文字转路径。 */
[[nodiscard]] std::filesystem::path pathFromUtf8(std::string_view utf8);

}  // namespace ember::win32::platform
