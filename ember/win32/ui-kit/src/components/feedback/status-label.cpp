/*
 * ------------------------------------------------------------------------
 * 名称：status-label.cpp
 * 说明：状态标签的实现：16 像素宽的图标槽（图标 14 像素、水平与垂直居中）、图标间距、文字；整体一行，高度是文字行高。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不换行；成功用状态色（成功），已停止用弱化色。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/status-label.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

StatusLabel::StatusLabel() {
  auto text = std::make_unique<Label>();
  text->setWrap(false);
  text_ = static_cast<Label*>(addChild(std::move(text)));
  setTone(StatusTone::Stopped);
}

void StatusLabel::setTone(StatusTone tone) {
  tone_ = tone;
  text_->setColorProvider([tone](ThemeName theme) { return tone == StatusTone::Success ? themeTokens(theme).status_success : themeTokens(theme).text_muted; });
  invalidate();
}

void StatusLabel::setText(std::string text) {
  text_->setText(std::move(text));
  invalidate();
}

Size StatusLabel::measure(Size /*available*/) {
  const Size size = text_->measure(Size{});
  // 高度是文字的行高（没有文字时为 0）
  return Size{tokens::icon_slot_size + tokens::icon_text_gap + size.width, size.height};
}

void StatusLabel::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double offset = tokens::icon_slot_size + tokens::icon_text_gap;
  text_->arrange(Rect{bounds.x + offset, bounds.y, std::max(0.0, bounds.width - offset), bounds.height});
}

void StatusLabel::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const bool success = tone_ == StatusTone::Success;
  const Rect icon{bounds().x + (tokens::icon_slot_size - tokens::icon_size) / 2.0, bounds().y + (bounds().height - tokens::icon_size) / 2.0, tokens::icon_size, tokens::icon_size};
  canvas.drawIcon(success ? IconId::CircleCheck : IconId::CircleX, icon, success ? colors.status_success : colors.text_muted, tokens::icon_stroke_width);
}

}  // namespace rujian::ui
