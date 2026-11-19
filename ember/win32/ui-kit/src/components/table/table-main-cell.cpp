/*
 * ------------------------------------------------------------------------
 * 名称：table-main-cell.cpp
 * 说明：主副文字单元格的实现：主文字可换行，说明单行、最宽 360；两者之间留 2 像素。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：说明的宽度取自己的文字宽度与 360 中较小的，不随可用宽度变化（用省略号裁剪），所以最小内容宽度里包含它。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/table.hpp"

namespace rujian::ui {

namespace {

/** 说明的最大宽度。 */
constexpr double kDescriptionMaxWidth = 360.0;
constexpr double kLineHeight = 1.45;

}  // namespace

TableMainCell::TableMainCell() {
  auto text = std::make_unique<Label>();
  text->setFontSize(tokens::table_title_font_size);
  text->setFontWeight(static_cast<int>(tokens::table_title_font_weight));
  text->setLineHeight(kLineHeight);
  text_ = static_cast<Label*>(addChild(std::move(text)));

  auto description = std::make_unique<Label>();
  description->setFontSize(tokens::table_compact_font_size);
  description->setLineHeight(kLineHeight);
  description->setWrap(false);
  description->setColorProvider([](ThemeName theme) { return themeTokens(theme).table_description_color; });
  description_ = static_cast<Label*>(addChild(std::move(description)));
}

void TableMainCell::setText(std::string text) {
  text_->setText(std::move(text));
  invalidate();
}

void TableMainCell::setDescription(std::string description) {
  description_->setText(std::move(description));
  invalidate();
}

Size TableMainCell::measure(Size available) {
  const Size text = text_->measure(available);
  const Size description = description_->measure(Size{});
  const double descriptionWidth = std::min(description.width, kDescriptionMaxWidth);
  const double gap = description.height > 0 ? tokens::space_xxs : 0.0;
  return Size{std::max(text.width, descriptionWidth), text.height + gap + description.height};
}

void TableMainCell::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Size text = text_->measure(Size{bounds.width, 0});
  text_->arrange(Rect{bounds.x, bounds.y, bounds.width, text.height});
  const Size description = description_->measure(Size{});
  const double width = std::min({description.width, kDescriptionMaxWidth, bounds.width});
  description_->arrange(Rect{bounds.x, bounds.y + text.height + tokens::space_xxs, width, description.height});
}

}  // namespace rujian::ui
