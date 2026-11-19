/*
 * ------------------------------------------------------------------------
 * 名称：system-theme.hpp
 * 说明：读取系统的明暗主题：“应用使用浅色主题”的设置。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：读 HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize 的 AppsUseLightTheme；没有这一项的系统（Windows 7、8 与早期 Windows 10）按浅色处理。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace ember::win32::platform {

/** 系统是否让应用使用浅色主题；读不到设置时为 true。 */
[[nodiscard]] bool systemUsesLightTheme();

}  // namespace ember::win32::platform
