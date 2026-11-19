/*
 * ------------------------------------------------------------------------
 * 名称：panel.cpp
 * 说明：面板的实现：卡片外观、标题行（标题居左、aside 居右、垂直居中）、可滚动的内容区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：标题行高度是“最小高度”（令牌）与“内容加上下内边距加下边线”中较大的一个（网页版的 border-box 规则）；
 *       内容区没有内容时不占高度，标题行的下边线也不画。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/panel.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/scroll-area.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;

}  // namespace

Panel::Panel() {
  auto title = std::make_unique<Label>();
  title->setFontSize(tokens::font_size_title);
  title->setFontWeight(static_cast<int>(tokens::title_font_weight));
  title_ = static_cast<Label*>(addChild(std::move(title)));

  auto body = std::make_unique<Stack>();
  body->setGap(tokens::space_md);
  body->setPadding(Insets{tokens::panel_padding_y, tokens::panel_padding_x, tokens::panel_padding_y, tokens::panel_padding_x});
  body_ = body.get();
  auto scroll = std::make_unique<ScrollArea>();
  scroll->setContent(std::move(body));
  scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));
}

void Panel::setTitle(std::string title) {
  title_->setText(std::move(title));
  invalidate();
}

void Panel::setAside(std::unique_ptr<Widget> aside) {
  if (aside_ != nullptr) removeChild(aside_);
  aside_ = aside ? addChild(std::move(aside)) : nullptr;
  invalidate();
}

void Panel::setCompactHead(bool compactHead) {
  compactHead_ = compactHead;
  invalidate();
}

void Panel::setPlain(bool plain) {
  plain_ = plain;
  invalidate();
}

Widget* Panel::addContent(std::unique_ptr<Widget> content) {
  Widget* added = body_->addChild(std::move(content));
  invalidate();
  return added;
}

double Panel::borderWidth() const { return plain_ ? 0.0 : snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

bool Panel::hasHead() const { return !title_->text().empty() || aside_ != nullptr; }

bool Panel::hasBody() const { return !body_->children().empty(); }

double Panel::headHeight(double innerWidth) {
  if (!hasHead()) return 0.0;
  const double paddingY = compactHead_ ? tokens::panel_compact_head_padding_y : tokens::panel_head_padding_y;
  const double minimum = compactHead_ ? tokens::panel_compact_header_height : tokens::panel_header_height;
  const double divider = hasBody() ? snapBorderWidth(kBorderWidth, host()->scale()) : 0.0;
  const double asideWidth = aside_ != nullptr ? aside_->measure(Size{}).width : 0.0;
  const double titleWidth = std::max(0.0, innerWidth - 2 * tokens::panel_padding_x - (aside_ != nullptr && !title_->text().empty() ? asideWidth + tokens::space_md : 0.0));
  const double titleHeight = title_->measure(Size{titleWidth, 0}).height;
  const double asideHeight = aside_ != nullptr ? aside_->measure(Size{}).height : 0.0;
  return std::max(minimum, std::max(titleHeight, asideHeight) + 2 * paddingY + divider);
}

Size Panel::measure(Size available) {
  const double border = borderWidth();
  double innerWidth = available.width > 0 ? std::max(0.0, available.width - 2 * border) : 0.0;
  if (available.width <= 0) {
    // 不限宽度时取标题行与内容区中较宽的
    double head = 0.0;
    if (hasHead()) {
      head = 2 * tokens::panel_padding_x + title_->measure(Size{}).width;
      if (aside_ != nullptr) head += aside_->measure(Size{}).width + (title_->text().empty() ? 0.0 : tokens::space_md);
    }
    const double body = hasBody() ? scroll_->measure(Size{}).width : 0.0;
    innerWidth = std::max(head, body);
  }
  const double bodyHeight = hasBody() ? scroll_->measure(Size{innerWidth, 0}).height : 0.0;
  return Size{innerWidth + 2 * border, headHeight(innerWidth) + bodyHeight + 2 * border};
}

void Panel::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const double border = borderWidth();
  const Rect inner = bounds.inset(Insets::all(border));
  const double head = headHeight(inner.width);
  if (hasHead()) {
    const double paddingY = compactHead_ ? tokens::panel_compact_head_padding_y : tokens::panel_head_padding_y;
    const double divider = hasBody() ? snapBorderWidth(kBorderWidth, host()->scale()) : 0.0;
    const double centerY = inner.y + paddingY + (head - 2 * paddingY - divider) / 2.0;
    const bool hasTitle = !title_->text().empty();
    const double asideWidth = aside_ != nullptr ? aside_->measure(Size{}).width : 0.0;
    const double asideHeight = aside_ != nullptr ? aside_->measure(Size{}).height : 0.0;
    const double titleWidth = std::max(0.0, inner.width - 2 * tokens::panel_padding_x - (aside_ != nullptr && hasTitle ? asideWidth + tokens::space_md : 0.0));
    const double titleHeight = title_->measure(Size{titleWidth, 0}).height;
    title_->setVisible(hasTitle);
    title_->arrange(Rect{inner.x + tokens::panel_padding_x, centerY - titleHeight / 2.0, titleWidth, titleHeight});
    if (aside_ != nullptr) {
      // 有标题时 aside 靠右，只有 aside 时靠左
      const double left = hasTitle ? inner.right() - tokens::panel_padding_x - asideWidth : inner.x + tokens::panel_padding_x;
      aside_->arrange(Rect{left, centerY - asideHeight / 2.0, asideWidth, asideHeight});
    }
  } else {
    title_->setVisible(false);
  }
  scroll_->setVisible(hasBody());
  scroll_->arrange(Rect{inner.x, inner.y + head, inner.width, std::max(0.0, inner.height - head)});
}

void Panel::paint(Canvas& canvas) {
  if (plain_) return;
  const auto& colors = themeTokens(host()->theme());
  canvas.drawShadow(bounds(), tokens::card_radius, colors.card_shadow);
  canvas.drawBox(bounds(), BoxStyle{.fill = colors.card_bg, .radius = tokens::card_radius, .borderWidth = kBorderWidth, .borderColor = colors.card_border});
}

void Panel::paintChildren(Canvas& canvas) {
  const auto& colors = themeTokens(host()->theme());
  const double border = borderWidth();
  const Rect inner = bounds().inset(Insets::all(border));
  const double radius = plain_ ? 0.0 : std::max(0.0, tokens::card_radius - border);
  canvas.pushClip(inner, radius);
  Widget::paintChildren(canvas);
  if (hasHead() && hasBody()) {
    const double divider = snapBorderWidth(kBorderWidth, host()->scale());
    canvas.fillRect(Rect{inner.x, inner.y + headHeight(inner.width) - divider, inner.width, divider}, colors.divider);
  }
  canvas.popClip();
}

}  // namespace rujian::ui
