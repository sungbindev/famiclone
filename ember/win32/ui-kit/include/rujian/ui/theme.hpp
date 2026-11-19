/*
 * ------------------------------------------------------------------------
 * 名称：theme.hpp
 * 说明：主题名称：深色与浅色两个主题，对应网页版 html 元素上的 theme-dark 与 theme-light。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：颜色取值在生成的令牌文件里，控件内部通过 base/theme-tokens.hpp 按主题取用。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace rujian::ui {

/** 主题。 */
enum class ThemeName { Dark, Light };

}  // namespace rujian::ui
