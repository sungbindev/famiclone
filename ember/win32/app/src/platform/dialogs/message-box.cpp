/*
 * ------------------------------------------------------------------------
 * 名称：message-box.cpp
 * 说明：启动阶段错误提示的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/dialogs/message-box.hpp"

#include <windows.h>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

void showErrorMessage(std::string_view title, std::string_view text) { MessageBoxW(nullptr, widen(text).c_str(), widen(title).c_str(), MB_OK | MB_ICONERROR); }

}  // namespace ember::win32::platform
