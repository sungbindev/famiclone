/*
 * ------------------------------------------------------------------------
 * 名称：table-main-cell.cpp
 * 说明：主副文字单元格的实现：主文字与说明都可换行，宽度随所在列；两者之间留 2 像素。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-22
 * 备注：最大内容宽度是两者不换行时的宽度，最小内容宽度由列的最小宽度与换行后的最窄情况决定。
 * ------------------------------------------------------------------------
 */

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/table.hpp"

namespace rujian::ui {

namespace {

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
  const Size description = description_->measure(available);
  const double gap = description.height > 0 ? tokens::space_xxs : 0.0;
  return Size{std::max(text.width, description.width), text.height + gap + description.height};
}

void TableMainCell::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Size text = text_->measure(Size{bounds.width, 0});
  text_->arrange(Rect{bounds.x, bounds.y, bounds.width, text.height});
  const Size description = description_->measure(Size{bounds.width, 0});
  description_->arrange(Rect{bounds.x, bounds.y + text.height + tokens::space_xxs, bounds.width, description.height});
}

}  // namespace rujian::ui
