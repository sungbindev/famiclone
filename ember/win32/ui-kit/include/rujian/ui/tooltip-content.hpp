/*
 * ------------------------------------------------------------------------
 * 名称：tooltip-content.hpp
 * 说明：悬停提示的内容：纯文字，或由标题行（图标加标题）与分条说明（图标、小标题、正文）组成的版式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应网页版的 title 属性与 setTooltip；控件用 Widget::setTooltip、setRichTooltip 设置，鼠标停在控件上一会儿后由悬停提示控件显示。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/icon-id.hpp"

namespace rujian::ui {

/** 版式提示里的一条说明。 */
struct TooltipItem {
  /** 条目左侧的图标。 */
  std::optional<IconId> icon;
  /** 小标题。 */
  std::string title;
  /** 正文。 */
  std::string text;
};

/** 悬停提示的内容。 */
struct TooltipContent {
  /** 纯文字提示的文字（可含换行）；版式提示不用它。空文字的纯文字提示表示“这里没有提示”，并盖住外层控件的提示。 */
  std::string text;
  /** 是否是版式提示。 */
  bool rich{false};
  /** 标题行的文字（版式提示）。 */
  std::string title;
  /** 标题行的图标（版式提示）。 */
  std::optional<IconId> icon;
  /** 分条说明（版式提示）。 */
  std::vector<TooltipItem> items;
};

}  // namespace rujian::ui
