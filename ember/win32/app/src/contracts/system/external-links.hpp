/*
 * ------------------------------------------------------------------------
 * 名称：external-links.hpp
 * 说明：外部链接的抽象：用系统的默认程序打开一个网址（项目地址之类）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只允许 http 与 https 开头的网址。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

namespace ember::win32::contracts {

/** 外部链接。 */
class ExternalLinks {
 public:
  virtual ~ExternalLinks() = default;

  /**
   * 打开网址。
   * @returns 成功时为空文字，失败时是错误说明。
   */
  [[nodiscard]] virtual std::string open(const std::string& url) = 0;
};

}  // namespace ember::win32::contracts
