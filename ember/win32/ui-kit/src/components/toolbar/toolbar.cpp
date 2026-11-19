/*
 * ------------------------------------------------------------------------
 * 名称：toolbar.cpp
 * 说明：工具栏的实现：一行里“前导加说明”靠左、操作靠右，各自在行内垂直居中；放不下时操作换到第二行（靠左）；高度取最小高度与内容加下内边距加下边线中较大的。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：行间距与前导、说明之间的间距都取工具栏间距。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/toolbar.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;

}  // namespace

Toolbar::Toolbar() {
  auto description = std::make_unique<Label>();
  description->setFontSize(tokens::toolbar_description_font_size);
  description->setWrap(false);
  description->setColorProvider([](ThemeName theme) { return themeTokens(theme).toolbar_description_color; });
  description_ = static_cast<Label*>(addChild(std::move(description)));
}

void Toolbar::setLeading(std::unique_ptr<Widget> leading) {
  if (leading_ != nullptr) removeChild(leading_);
  leading_ = leading ? addChild(std::move(leading)) : nullptr;
  invalidate();
}

void Toolbar::setDescription(std::string description) {
  description_->setText(std::move(description));
  invalidate();
}

void Toolbar::setActions(std::unique_ptr<Widget> actions) {
  if (actions_ != nullptr) removeChild(actions_);
  actions_ = actions ? addChild(std::move(actions)) : nullptr;
  invalidate();
}

double Toolbar::borderWidth() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

Toolbar::Layout Toolbar::layout(double width) {
  Layout result;
  if (leading_ != nullptr) {
    const Size size = leading_->measure(Size{});
    result.leadingWidth = size.width;
    result.leadingHeight = size.height;
  }
  const Size description = description_->measure(Size{});
  result.descriptionWidth = description.width;
  result.descriptionHeight = description.height;
  if (actions_ != nullptr) {
    const Size size = actions_->measure(Size{});
    result.actionsWidth = size.width;
    result.actionsHeight = size.height;
  }
  double startWidth = result.leadingWidth + result.descriptionWidth;
  if (leading_ != nullptr && result.descriptionWidth > 0) startWidth += tokens::toolbar_gap;
  result.startHeight = std::max(result.leadingHeight, result.descriptionHeight);
  const bool hasStart = leading_ != nullptr || result.descriptionWidth > 0;
  // 放不下时操作换到第二行
  result.wrapped = hasStart && actions_ != nullptr && startWidth + tokens::toolbar_gap + result.actionsWidth > width;
  double content = 0.0;
  if (result.wrapped) content = result.startHeight + tokens::toolbar_gap + result.actionsHeight;
  else content = std::max(result.startHeight, result.actionsHeight);
  result.height = std::max(tokens::toolbar_min_height, content + tokens::toolbar_padding_bottom + borderWidth());
  return result;
}

Size Toolbar::measure(Size available) {
  const Layout laid = layout(available.width > 0 ? available.width : 1e9);
  return Size{available.width > 0 ? available.width : laid.leadingWidth + laid.descriptionWidth + laid.actionsWidth, laid.height};
}

void Toolbar::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Layout laid = layout(bounds.width);
  const double contentHeight = laid.height - tokens::toolbar_padding_bottom - borderWidth();
  // 第一行的高度：放得下时是整个内容高度，换行时是前导加说明那一行
  const double firstLine = laid.wrapped ? laid.startHeight : contentHeight;
  const auto center = [&](double lineHeight, double height) { return bounds.y + (lineHeight - height) / 2.0; };
  double x = bounds.x;
  if (leading_ != nullptr) {
    leading_->arrange(Rect{x, center(firstLine, laid.leadingHeight), laid.leadingWidth, laid.leadingHeight});
    x += laid.leadingWidth + tokens::toolbar_gap;
  }
  description_->arrange(Rect{x, center(firstLine, laid.descriptionHeight), laid.descriptionWidth, laid.descriptionHeight});
  if (actions_ != nullptr) {
    if (laid.wrapped) {
      actions_->arrange(Rect{bounds.x, bounds.y + laid.startHeight + tokens::toolbar_gap, laid.actionsWidth, laid.actionsHeight});
    } else {
      actions_->arrange(Rect{bounds.right() - laid.actionsWidth, center(firstLine, laid.actionsHeight), laid.actionsWidth, laid.actionsHeight});
    }
  }
}

void Toolbar::paint(Canvas& canvas) {
  const double border = borderWidth();
  canvas.fillRect(Rect{bounds().x, bounds().bottom() - border, bounds().width, border}, themeTokens(host()->theme()).toolbar_border);
}

}  // namespace rujian::ui
