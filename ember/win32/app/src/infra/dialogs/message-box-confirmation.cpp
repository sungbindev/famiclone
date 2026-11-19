/*
 * ------------------------------------------------------------------------
 * 名称：message-box-confirmation.cpp
 * 说明：用系统消息框实现的确认询问的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/dialogs/message-box-confirmation.hpp"

#include "platform/dialogs/message-box.hpp"

namespace ember::win32::infra {

bool MessageBoxConfirmation::ask(const std::string& title, const std::string& text) { return platform::askYesNo(title, text); }

}  // namespace ember::win32::infra
