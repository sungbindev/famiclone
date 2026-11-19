/*
 * ------------------------------------------------------------------------
 * 名称：shell-external-links.hpp
 * 说明：用系统默认浏览器打开网址的外部链接。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只允许 http 与 https 开头的网址。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/system/external-links.hpp"

namespace ember::win32::infra {

/** 用系统默认浏览器打开网址。 */
class ShellExternalLinks final : public contracts::ExternalLinks {
 public:
  [[nodiscard]] std::string open(const std::string& url) override;
};

}  // namespace ember::win32::infra
