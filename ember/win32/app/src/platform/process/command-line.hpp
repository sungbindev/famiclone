/*
 * ------------------------------------------------------------------------
 * 名称：command-line.hpp
 * 说明：把 Windows 命令行拆成 UTF-8 参数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：按系统的命令行规则拆分（引号、反斜杠与 CommandLineToArgvW 一致）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

namespace ember::win32::platform {

/**
 * 把完整的命令行拆成参数。
 * @param commandLine 命令行文字，第一项是程序名。
 * @returns UTF-8 参数，不含程序名；拆不开时为空。
 */
[[nodiscard]] std::vector<std::string> splitCommandLine(const std::wstring& commandLine);

}  // namespace ember::win32::platform
