/*
 * ------------------------------------------------------------------------
 * 名称：theme.cpp
 * 说明：主题相关的公共函数：应用画窗口外框要用的颜色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/theme.hpp"

#include "base/theme-tokens.hpp"

namespace rujian::ui {

Color windowBorderColor(ThemeName theme) { return themeTokens(theme).chrome_border; }

}  // namespace rujian::ui
