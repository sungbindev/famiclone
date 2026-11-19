/*
 * ------------------------------------------------------------------------
 * 名称：field.cpp
 * 说明：字段的实现：按块排版，各部分之间的间距就是网页版里各元素的下外边距（标签 2，说明 6；无标签时说明在控件下方，上外边距 4）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：控件接受容器给的宽度时铺满宽度，否则取自己的宽度，靠左。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/field.hpp"

#include <algorithm>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

/** 竖排的一项：控件与它上方留出的间距。 */
struct Step {
  Widget* widget;
  double before;
};

/** 按有无标签与说明排出竖排的各项：有标签时依次是标签、说明、控件，标签后留 2、说明后留 6；没有标签时是控件、说明（上方留 4）。 */
std::vector<Step> stepsOf(Label* label, Label* description, Widget* control) {
  const bool hasLabel = !label->text().empty();
  const bool hasDescription = !description->text().empty();
  std::vector<Step> result;
  if (hasLabel) result.push_back(Step{label, 0.0});
  if (hasLabel && hasDescription) result.push_back(Step{description, tokens::field_label_gap});
  if (control != nullptr) {
    const double before = !hasLabel ? 0.0 : (hasDescription ? tokens::field_description_gap : tokens::field_label_gap);
    result.push_back(Step{control, before});
  }
  if (!hasLabel && hasDescription) result.push_back(Step{description, tokens::space_xs});
  return result;
}

}  // namespace

Field::Field() {
  auto label = std::make_unique<Label>();
  label->setFontSize(tokens::field_font_size);
  label->setFontWeight(static_cast<int>(tokens::field_label_weight));
  label->setColorProvider([](ThemeName theme) { return themeTokens(theme).field_label_color; });
  label_ = static_cast<Label*>(addChild(std::move(label)));

  auto description = std::make_unique<Label>();
  description->setFontSize(tokens::field_description_size);
  description->setLineHeight(tokens::field_description_line_height);
  description->setColorProvider([](ThemeName theme) { return themeTokens(theme).field_description_color; });
  description_ = static_cast<Label*>(addChild(std::move(description)));
}

void Field::setLabel(std::string label) {
  label_->setText(std::move(label));
  invalidate();
}

void Field::setDescription(std::string description) {
  description_->setText(std::move(description));
  invalidate();
}

void Field::setControl(std::unique_ptr<Widget> control) {
  if (control_ != nullptr) removeChild(control_);
  control_ = control ? addChild(std::move(control)) : nullptr;
  invalidate();
}

Size Field::measure(Size available) {
  double height = 0.0;
  double widest = 0.0;
  for (const Step& step : stepsOf(label_, description_, control_)) {
    const Size size = step.widget->measure(Size{available.width, 0});
    height += step.before + size.height;
    widest = std::max(widest, size.width);
  }
  return Size{available.width > 0 ? available.width : widest, height};
}

void Field::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  label_->setVisible(!label_->text().empty());
  description_->setVisible(!description_->text().empty());
  double y = bounds.y;
  for (const Step& step : stepsOf(label_, description_, control_)) {
    const Size size = step.widget->measure(Size{bounds.width, 0});
    y += step.before;
    step.widget->arrange(Rect{bounds.x, y, step.widget->stretchesWidth() ? bounds.width : size.width, size.height});
    y += size.height;
  }
}

}  // namespace rujian::ui
