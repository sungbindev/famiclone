/*
 * ------------------------------------------------------------------------
 * 名称：state.cpp
 * 说明：说明块的实现：文字取说明字号与行高；出错时图标放在 16 像素宽的槽里，图标 14 像素、槽内水平居中、垂直对着第一行文字居中。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：图标槽与文字之间留图标间距；图标颜色同文字（错误色）。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/state.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

/** 说明文字的行高（逻辑像素）：字号乘行高倍数。 */
constexpr double kLineHeight = tokens::state_text_font_size * tokens::state_text_line_height;

}  // namespace

State::State() {
  auto text = std::make_unique<Label>();
  text->setFontSize(tokens::state_text_font_size);
  text->setLineHeight(tokens::state_text_line_height);
  text_ = static_cast<Label*>(addChild(std::move(text)));
  setError(false);
}

void State::setText(std::string text) {
  text_->setText(std::move(text));
  invalidate();
}

void State::setError(bool error) {
  error_ = error;
  text_->setColorProvider([error](ThemeName theme) { return error ? themeTokens(theme).status_error : themeTokens(theme).state_text_color; });
  invalidate();
}

double State::textOffset() const { return error_ ? tokens::icon_slot_size + tokens::icon_text_gap : 0.0; }

Size State::measure(Size available) {
  const double textWidth = available.width > 0 ? std::max(0.0, available.width - textOffset()) : 0.0;
  const Size size = text_->measure(Size{textWidth, 0});
  return Size{available.width > 0 ? available.width : size.width + textOffset(), size.height};
}

void State::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double width = std::max(0.0, bounds.width - textOffset());
  text_->arrange(Rect{bounds.x + textOffset(), bounds.y, width, text_->measure(Size{width, 0}).height});
}

void State::paint(Canvas& canvas) {
  if (!error_ || text_->text().empty()) return;
  // 图标槽顶边比第一行文字的顶边高 (槽高 - 行高) / 2，图标在槽里水平居中、垂直居中
  const double slotTop = bounds().y + (kLineHeight - tokens::icon_slot_size) / 2.0;
  const Rect icon{bounds().x + (tokens::icon_slot_size - tokens::icon_size) / 2.0, slotTop + (tokens::icon_slot_size - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size};
  canvas.drawIcon(IconId::AlertCircle, icon, themeTokens(host()->theme()).status_error, tokens::icon_stroke_width);
}

}  // namespace rujian::ui
