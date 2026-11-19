/*
 * ------------------------------------------------------------------------
 * 名称：url-opener.hpp
 * 说明：用系统默认的程序打开网址，以及判断系统默认的浏览器是不是 Internet Explorer。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只打开 http 与 https 开头的网址；Windows 7 上默认浏览器可能是 Internet Explorer，帮助文档站点需要较新的浏览器，调用方据此提示用户。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <string_view>

namespace ember::win32::platform {

/**
 * 用默认浏览器打开网址。
 * @param url UTF-8 网址，必须以 http:// 或 https:// 开头。
 * @returns 成功时为空文字，失败时是错误说明。
 */
[[nodiscard]] std::string openUrlInBrowser(std::string_view url);

/** 系统默认的浏览器是不是 Internet Explorer。 */
[[nodiscard]] bool defaultBrowserIsInternetExplorer();

}  // namespace ember::win32::platform
