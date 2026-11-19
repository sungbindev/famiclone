/*
 * ------------------------------------------------------------------------
 * 名称：setting-row.cpp
 * 说明：设置行的实现：两列（名称说明列最宽 420、控件列最宽 280，空间不够时两列平分再让给名称列），各列在行内垂直居中；窄窗口时上下排列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：行的上下内边距取令牌；名称与说明之间留 2，控件与错误文字之间留 4，上下排列时两部分之间留中间距。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/setting-row.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/label.hpp"

namespace rujian::ui {

namespace {

/** 窗口宽度不超过它时两部分上下排列（与组件库样式里的媒体查询一致）。 */
constexpr double kStackedViewportWidth = 720.0;

}  // namespace

SettingRow::SettingRow() {
  auto label = std::make_unique<Label>();
  label->setFontSize(tokens::setting_row_label_font_size);
  label->setFontWeight(static_cast<int>(tokens::setting_row_label_font_weight));
  label_ = static_cast<Label*>(addChild(std::move(label)));

  auto description = std::make_unique<Label>();
  description->setFontSize(tokens::setting_row_description_font_size);
  description->setLineHeight(tokens::setting_row_description_line_height);
  description->setColorProvider([](ThemeName theme) { return themeTokens(theme).setting_row_description_color; });
  description_ = static_cast<Label*>(addChild(std::move(description)));

  auto error = std::make_unique<Label>();
  error->setFontSize(tokens::field_error_size);
  error->setColorProvider([](ThemeName theme) { return themeTokens(theme).status_error; });
  error_ = static_cast<Label*>(addChild(std::move(error)));
}

void SettingRow::setLabel(std::string label) {
  label_->setText(std::move(label));
  invalidate();
}

void SettingRow::setDescription(std::string description) {
  description_->setText(std::move(description));
  invalidate();
}

void SettingRow::setControl(std::unique_ptr<Widget> control) {
  if (control_ != nullptr) removeChild(control_);
  control_ = control ? addChild(std::move(control)) : nullptr;
  invalidate();
}

void SettingRow::setError(std::string error) {
  error_->setText(std::move(error));
  invalidate();
}

double SettingRow::paddingTop() const { return trimTop() ? 0.0 : tokens::setting_row_padding_y; }

double SettingRow::paddingBottom() const { return trimBottom() ? 0.0 : tokens::setting_row_padding_y; }

SettingRow::Layout SettingRow::layout(double width) {
  Layout result;
  result.stacked = host() != nullptr && host()->viewportSize().width <= kStackedViewportWidth;
  if (result.stacked) {
    result.textWidth = width;
    result.controlWidth = width;
  } else {
    // 两列的宽度范围是 0 到最大值：空间不够时从 0 起平分，控件列先到上限
    const double available = std::max(0.0, width - tokens::space_page);
    result.controlWidth = std::min(tokens::setting_row_control_width, available / 2.0);
    result.textWidth = std::min(tokens::setting_row_text_width, available - result.controlWidth);
  }
  result.labelHeight = label_->measure(Size{result.textWidth, 0}).height;
  result.descriptionHeight = description_->measure(Size{result.textWidth, 0}).height;
  result.textHeight = result.labelHeight + (result.descriptionHeight > 0 ? tokens::space_xxs + result.descriptionHeight : 0.0);
  result.controlHeight = control_ != nullptr ? control_->measure(Size{result.controlWidth, 0}).height : 0.0;
  result.errorHeight = error_->measure(Size{result.controlWidth, 0}).height;
  result.controlColumnHeight = result.controlHeight + (result.errorHeight > 0 ? tokens::space_xs + result.errorHeight : 0.0);
  const double content = result.stacked ? result.textHeight + tokens::space_md + result.controlColumnHeight : std::max(result.textHeight, result.controlColumnHeight);
  result.height = content + paddingTop() + paddingBottom();
  return result;
}

Size SettingRow::measure(Size available) {
  const double width = available.width > 0 ? available.width : tokens::setting_row_text_width + tokens::space_page + tokens::setting_row_control_width;
  return Size{width, layout(width).height};
}

void SettingRow::arrange(const Rect& bounds) {
  setBounds(bounds);
  const Layout laid = layout(bounds.width);
  const double top = bounds.y + paddingTop();
  const double contentHeight = laid.height - paddingTop() - paddingBottom();
  const double textTop = laid.stacked ? top : top + (contentHeight - laid.textHeight) / 2.0;
  label_->arrange(Rect{bounds.x, textTop, laid.textWidth, laid.labelHeight});
  description_->arrange(Rect{bounds.x, textTop + laid.labelHeight + tokens::space_xxs, laid.textWidth, laid.descriptionHeight});

  const double controlLeft = laid.stacked ? bounds.x : bounds.x + laid.textWidth + tokens::space_page;
  const double controlTop = laid.stacked ? top + laid.textHeight + tokens::space_md : top + (contentHeight - laid.controlColumnHeight) / 2.0;
  if (control_ != nullptr) {
    const Size size = control_->measure(Size{laid.controlWidth, 0});
    control_->arrange(Rect{controlLeft, controlTop, control_->stretchesWidth() ? laid.controlWidth : size.width, laid.controlHeight});
  }
  error_->arrange(Rect{controlLeft, controlTop + laid.controlHeight + tokens::space_xs, laid.controlWidth, laid.errorHeight});
}

}  // namespace rujian::ui
