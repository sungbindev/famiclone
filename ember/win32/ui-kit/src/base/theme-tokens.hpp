/*
 * ------------------------------------------------------------------------
 * 名称：theme-tokens.hpp
 * 说明：按主题取颜色令牌的内部入口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：令牌由 tools/gen-tokens.mjs 生成；控件只经这里取令牌，不直接引用生成文件。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "generated/tokens.generated.hpp"
#include "rujian/ui/theme.hpp"

namespace rujian::ui {

/** 取指定主题的全部颜色令牌。 */
[[nodiscard]] inline const tokens::ThemeTokens& themeTokens(ThemeName theme) { return theme == ThemeName::Light ? tokens::light : tokens::dark; }

}  // namespace rujian::ui
