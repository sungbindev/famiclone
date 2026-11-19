/*
 * ------------------------------------------------------------------------
 * 名称：shell-external-links.cpp
 * 说明：用系统默认浏览器打开网址的外部链接的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/links/shell-external-links.hpp"

#include "platform/shell/url-opener.hpp"

namespace ember::win32::infra {

std::string ShellExternalLinks::open(const std::string& url) { return platform::openUrlInBrowser(url); }

}  // namespace ember::win32::infra
