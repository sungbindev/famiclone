/*
 * ------------------------------------------------------------------------
 * 名称：message-box-confirmation.hpp
 * 说明：用系统消息框实现的确认询问：弹出“是/否”消息框，等用户回答。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/confirmation.hpp"

namespace ember::win32::infra {

/** 用系统消息框询问。 */
class MessageBoxConfirmation final : public contracts::Confirmation {
 public:
  [[nodiscard]] bool ask(const std::string& title, const std::string& text) override;
};

}  // namespace ember::win32::infra
