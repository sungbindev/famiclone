/*
 * ------------------------------------------------------------------------
 * 名称：label.cpp
 * 说明：文字控件的实现：度量与绘制都交给宿主与画布，行高 normal 的取整规则在排版引擎里。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：没有文字时高度为 0（网页版里空的块没有行盒）。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/label.hpp"

#include "base/theme-tokens.hpp"

namespace rujian::ui {

Label::Label(std::string text) : text_(std::move(text)), fontSize_(tokens::font_size_body) {}

void Label::setText(std::string text) {
  text_ = std::move(text);
  invalidate();
}

void Label::setFontSize(double fontSize) {
  fontSize_ = fontSize;
  invalidate();
}

void Label::setFontWeight(int fontWeight) {
  fontWeight_ = fontWeight;
  invalidate();
}

void Label::setLineHeight(double lineHeight) {
  lineHeight_ = lineHeight;
  invalidate();
}

void Label::setWrap(bool wrap) {
  wrap_ = wrap;
  invalidate();
}

void Label::setAlign(TextAlign align) {
  align_ = align;
  invalidate();
}

void Label::setColorProvider(ColorProvider provider) {
  color_ = std::move(provider);
  invalidate();
}

void Label::setMuted(bool muted) {
  setColorProvider(muted ? ColorProvider([](ThemeName theme) { return themeTokens(theme).text_muted; }) : ColorProvider());
}

TextStyle Label::style(Color color) const {
  TextStyle result;
  result.fontSize = fontSize_;
  result.fontWeight = fontWeight_;
  result.lineHeight = lineHeight_;
  result.color = color;
  result.wrap = wrap_;
  result.align = align_;
  result.overflow = wrap_ ? TextOverflow::Clip : TextOverflow::Ellipsis;
  return result;
}

Size Label::measure(Size available) {
  if (host() == nullptr || text_.empty()) return Size{};
  const TextMetrics metrics = host()->measureText(text_, style(kTransparent), wrap_ ? available.width : 0.0);
  return Size{metrics.width, metrics.height};
}

void Label::paint(Canvas& canvas) {
  if (text_.empty()) return;
  const ThemeName theme = host()->theme();
  Color color = color_ ? color_(theme) : kCurrentColor;
  // currentColor 取页面的正文文字色
  if (color == kCurrentColor) color = themeTokens(theme).text;
  canvas.drawText(text_, bounds(), style(color), VerticalAlign::Top);
}

Heading::Heading(std::string text) : Label(std::move(text)) {
  setFontSize(tokens::font_size_heading);
  setFontWeight(static_cast<int>(tokens::heading_font_weight));
}

}  // namespace rujian::ui
