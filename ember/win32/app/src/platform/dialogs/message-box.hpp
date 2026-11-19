/*
 * ------------------------------------------------------------------------
 * 名称：message-box.hpp
 * 说明：系统的消息框：启动阶段主窗口还没有时的错误提示，以及需要用户回答“是/否”的询问。
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

/** 弹出带问号图标的“是/否”消息框（归属当前活动窗口），用户点“是”时返回 true。 */
[[nodiscard]] bool askYesNo(std::string_view title, std::string_view text);

}  // namespace ember::win32::platform
