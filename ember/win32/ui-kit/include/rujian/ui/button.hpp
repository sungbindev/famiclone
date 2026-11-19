/*
 * ------------------------------------------------------------------------
 * 名称：button.hpp
 * 说明：按钮控件，对应网页版组件库的 Button。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：本阶段只实现基础样式（次要、主要）、图标、禁用与焦点环，用来验证控件库的绘制与布局；其余样式与属性按用法表在后续阶段补齐。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <optional>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 按钮样式。 */
enum class ButtonVariant { Secondary, Primary };

/** 按钮。 */
class Button : public Widget {
 public:
  /** @param label 按钮文字（UTF-8）。 */
  explicit Button(std::string label);

  /** 设置文字。 */
  void setLabel(std::string label);
  /** 设置样式。 */
  void setVariant(ButtonVariant variant);
  /** 设置文字前面的图标。 */
  void setIcon(std::optional<IconId> icon);
  /** 设置点击回调：鼠标在按钮上按下并抬起，或按 Enter、Space 键时触发。 */
  void setOnClick(std::function<void()> handler) { onClick_ = std::move(handler); }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onKeyUp(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  void click();

  std::string label_;
  ButtonVariant variant_{ButtonVariant::Secondary};
  std::optional<IconId> icon_;
  std::function<void()> onClick_;
  /** Space 键是否按在按钮上（抬起时才触发点击）。 */
  bool spaceDown_{false};
};

}  // namespace rujian::ui
