/*
 * ------------------------------------------------------------------------
 * 名称：confirmation.hpp
 * 说明：确认询问的端口：弹出一个是或否的询问窗口，等用户回答。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：由界面所在的层实现。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

namespace ember::win32::contracts {

/** 确认询问。 */
class Confirmation {
 public:
  virtual ~Confirmation() = default;

  /**
   * 询问用户“是”或“否”。
   * @param title 窗口标题。
   * @param text 询问的内容。
   * @returns 用户选了“是”时为 true。
   */
  [[nodiscard]] virtual bool ask(const std::string& title, const std::string& text) = 0;
};

}  // namespace ember::win32::contracts
