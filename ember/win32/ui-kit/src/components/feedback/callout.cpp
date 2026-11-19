/*
 * ------------------------------------------------------------------------
 * 名称：callout.cpp
 * 说明：提示块的实现：圆角框，底色是主色的 8%、边框是主色的 35%，内边距取令牌，内容纵向排列、间距为小间距。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：提示样式的主色是链接色，错误样式是错误色；说明文字颜色取提示块令牌。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/callout.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;
/** 底色与边框占主色的比例（网页版里是 color-mix 的 8% 与 35%）。 */
constexpr double kFillOpacity = 0.08;
constexpr double kBorderOpacity = 0.35;

}  // namespace

Callout::Callout() {
  auto text = std::make_unique<Label>();
  text->setFontSize(tokens::callout_font_size);
  text->setLineHeight(tokens::callout_line_height);
  text->setColorProvider([](ThemeName theme) { return themeTokens(theme).callout_text_color; });
  text_ = static_cast<Label*>(addChild(std::move(text)));
}

void Callout::setText(std::string text) {
  text_->setText(std::move(text));
  invalidate();
}

void Callout::setError(bool error) {
  error_ = error;
  invalidate();
}

Widget* Callout::addContent(std::unique_ptr<Widget> content) {
  Widget* added = addChild(std::move(content));
  invalidate();
  return added;
}

double Callout::borderWidth() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

Size Callout::measure(Size available) {
  const double border = borderWidth();
  const double innerWidth = available.width > 0 ? std::max(0.0, available.width - 2 * border - 2 * tokens::callout_padding_x) : 0.0;
  double height = 0.0;
  double widest = 0.0;
  bool first = true;
  for (const auto& child : children()) {
    const Size size = child->measure(Size{innerWidth, 0});
    if (size.height <= 0 && size.width <= 0) continue;
    height += (first ? 0.0 : tokens::space_xs) + size.height;
    widest = std::max(widest, size.width);
    first = false;
  }
  const double width = available.width > 0 ? available.width : widest + 2 * border + 2 * tokens::callout_padding_x;
  return Size{width, height + 2 * border + 2 * tokens::callout_padding_y};
}

void Callout::arrange(const Rect& bounds) {
  setBounds(bounds);
  const double border = borderWidth();
  const double left = bounds.x + border + tokens::callout_padding_x;
  const double innerWidth = std::max(0.0, bounds.width - 2 * border - 2 * tokens::callout_padding_x);
  double y = bounds.y + border + tokens::callout_padding_y;
  bool first = true;
  for (const auto& child : children()) {
    const Size size = child->measure(Size{innerWidth, 0});
    if (size.height <= 0 && size.width <= 0) {
      child->arrange(Rect{left, y, 0, 0});
      continue;
    }
    if (!first) y += tokens::space_xs;
    child->arrange(Rect{left, y, child->stretchesWidth() ? innerWidth : size.width, size.height});
    y += size.height;
    first = false;
  }
}

void Callout::paint(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const Color main = error_ ? colors.status_error : colors.link;
  canvas.drawBox(bounds(), BoxStyle{.fill = main.withOpacity(kFillOpacity), .radius = tokens::radius_lg, .borderWidth = kBorderWidth, .borderColor = main.withOpacity(kBorderOpacity)});
}

}  // namespace rujian::ui
