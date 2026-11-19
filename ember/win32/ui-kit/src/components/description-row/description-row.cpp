/*
 * ------------------------------------------------------------------------
 * 名称：description-row.cpp
 * 说明：说明行的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：说明文字的字号、行高与颜色取字段说明的令牌（--field-description-*）。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/description-row.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

DescriptionRow::DescriptionRow() {
  auto text = std::make_unique<Label>();
  text->setFontSize(tokens::field_description_size);
  text->setLineHeight(tokens::field_description_line_height);
  text->setColorProvider([](ThemeName theme) { return themeTokens(theme).field_description_color; });
  text_ = static_cast<Label*>(addChild(std::move(text)));
}

void DescriptionRow::setText(std::string text) {
  text_->setText(std::move(text));
  invalidate();
}

const std::string& DescriptionRow::text() const { return text_->text(); }

void DescriptionRow::setAction(std::unique_ptr<Widget> action) {
  if (action_ != nullptr) removeChild(action_);
  action_ = action ? addChild(std::move(action)) : nullptr;
  invalidate();
}

Size DescriptionRow::measure(Size available) {
  const Size action = action_ != nullptr ? action_->measure(Size{}) : Size{};
  const double gap = action.width > 0 ? tokens::space_md : 0.0;
  const double textWidth = available.width > 0 ? std::max(0.0, available.width - action.width - gap) : 0.0;
  const Size text = text_->measure(Size{textWidth, 0});
  const double width = available.width > 0 ? available.width : text.width + gap + action.width;
  return Size{width, std::max(text.height, action.height)};
}

void DescriptionRow::arrange(const Rect& bounds) {
  setBounds(bounds);
  const Size action = action_ != nullptr ? action_->measure(Size{}) : Size{};
  const double gap = action.width > 0 ? tokens::space_md : 0.0;
  const double textWidth = std::max(0.0, bounds.width - action.width - gap);
  const double textHeight = text_->measure(Size{textWidth, 0}).height;
  const double height = std::max(textHeight, action.height);
  text_->arrange(Rect{bounds.x, bounds.y + (height - textHeight) / 2.0, textWidth, textHeight});
  if (action_ != nullptr) action_->arrange(Rect{bounds.right() - action.width, bounds.y + (height - action.height) / 2.0, action.width, action.height});
}

}  // namespace rujian::ui
