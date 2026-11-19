/*
 * ------------------------------------------------------------------------
 * 名称：theme.hpp
 * 说明：主题名称：深色与浅色两个主题，对应网页版 html 元素上的 theme-dark 与 theme-light；以及应用画窗口外框要用的颜色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色取值在生成的令牌文件里，控件内部通过 base/theme-tokens.hpp 按主题取用；应用只能拿到这里给出的几个。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "rujian/ui/color.hpp"

namespace rujian::ui {

/** 主题。 */
enum class ThemeName { Dark, Light };

/** 窗口外框线的颜色（令牌 --chrome-border）：系统不画阴影的窗口用它自绘 1 像素边框。 */
[[nodiscard]] Color windowBorderColor(ThemeName theme);

}  // namespace rujian::ui
