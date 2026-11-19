/*
 * ------------------------------------------------------------------------
 * 名称：message-box.hpp
 * 说明：启动阶段的错误提示：主窗口还没有时用系统的消息框告诉用户为什么无法启动。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字是 UTF-8；只在启动失败时使用，运行中的提示用通知。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string_view>

namespace ember::win32::platform {

/** 弹出带错误图标的消息框，等用户点“确定”。 */
void showErrorMessage(std::string_view title, std::string_view text);

}  // namespace ember::win32::platform
