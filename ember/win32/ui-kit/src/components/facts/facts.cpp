/*
 * ------------------------------------------------------------------------
 * 名称：facts.cpp
 * 说明：键值清单的实现：宽窗口是两列（名称列固定宽，内容列占满剩余），窄窗口是上下排列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：两列时每行高度取名称与内容中较高的，行间距取令牌；上下排列时没有行间距，每条内容后留小间距。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/facts.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

/** 窗口宽度不超过它时名称与内容上下排列（与组件库样式里的媒体查询一致）。 */
constexpr double kNarrowViewportWidth = 420.0;

/** 内容控件的宽度：接受容器宽度的填满列宽，其他（如链接按钮）取自己的宽度，不超过列宽。 */
double valueWidthOf(Widget& value, double columnWidth) {
  if (value.stretchesWidth()) return columnWidth;
  return std::min(columnWidth, value.measure(Size{columnWidth, 0}).width);
}

}  // namespace

void Facts::addFact(std::string label, std::string value) {
  auto text = std::make_unique<Label>(std::move(value));
  text->setFontSize(tokens::facts_value_font_size);
  addFact(std::move(label), std::move(text));
}

void Facts::addFact(std::string label, std::unique_ptr<Widget> value) {
  auto name = std::make_unique<Label>(std::move(label));
  name->setFontSize(tokens::facts_label_font_size);
  name->setColorProvider([](ThemeName theme) { return themeTokens(theme).facts_label_color; });
  Widget* nameRaw = addChild(std::move(name));
  Widget* valueRaw = addChild(std::move(value));
  facts_.push_back(Fact{nameRaw, valueRaw});
  invalidate();
}

bool Facts::narrow() const { return host() != nullptr && host()->viewportSize().width <= kNarrowViewportWidth; }

Facts::Layout Facts::layout(double width) {
  Layout result;
  result.narrow = narrow();
  const double labelWidth = result.narrow ? width : tokens::facts_label_width;
  result.valueWidth = result.narrow ? width : std::max(0.0, width - tokens::facts_label_width - tokens::space_page);
  for (const Fact& fact : facts_) {
    result.labelHeights.push_back(fact.label->measure(Size{labelWidth, 0}).height);
    result.valueHeights.push_back(fact.value->measure(Size{result.valueWidth, 0}).height);
  }
  for (std::size_t i = 0; i < facts_.size(); ++i) {
    if (result.narrow) {
      result.height += result.labelHeights[i] + result.valueHeights[i] + tokens::space_sm;
    } else {
      result.height += std::max(result.labelHeights[i], result.valueHeights[i]) + (i > 0 ? tokens::facts_row_gap : 0.0);
    }
  }
  return result;
}

Size Facts::measure(Size available) {
  double width = available.width;
  if (width <= 0) {
    // 不限宽度：取最宽的内容
    double widest = 0.0;
    for (const Fact& fact : facts_) widest = std::max(widest, fact.value->measure(Size{}).width);
    width = narrow() ? widest : tokens::facts_label_width + tokens::space_page + widest;
  }
  return Size{width, layout(width).height};
}

Size Facts::minimumSize() {
  double widest = 0.0;
  for (const Fact& fact : facts_) widest = std::max(widest, fact.value->measure(Size{}).width);
  return Size{tokens::facts_label_width + tokens::space_page + widest, 0.0};
}

void Facts::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Layout laid = layout(bounds.width);
  double y = bounds.y;
  for (std::size_t i = 0; i < facts_.size(); ++i) {
    const Fact& fact = facts_[i];
    if (laid.narrow) {
      fact.label->arrange(Rect{bounds.x, y, bounds.width, laid.labelHeights[i]});
      y += laid.labelHeights[i];
      fact.value->arrange(Rect{bounds.x, y, valueWidthOf(*fact.value, bounds.width), laid.valueHeights[i]});
      y += laid.valueHeights[i] + tokens::space_sm;
    } else {
      if (i > 0) y += tokens::facts_row_gap;
      fact.label->arrange(Rect{bounds.x, y, tokens::facts_label_width, laid.labelHeights[i]});
      fact.value->arrange(Rect{bounds.x + tokens::facts_label_width + tokens::space_page, y, valueWidthOf(*fact.value, laid.valueWidth), laid.valueHeights[i]});
      y += std::max(laid.labelHeights[i], laid.valueHeights[i]);
    }
  }
}

}  // namespace rujian::ui
