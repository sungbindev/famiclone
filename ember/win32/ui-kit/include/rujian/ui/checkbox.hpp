/*
 * ------------------------------------------------------------------------
 * 名称：checkbox.hpp
 * 说明：复选框控件，对应网页版组件库的 Checkbox：一个方框加一段文字，点击或按空格切换勾选。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现 Ember 界面用到的属性（label、checked、onChange、disabled）；
 *       网页版的 invalid 只影响可访问性、没有视觉变化，这里不做；整行（方框加文字）都是点击与悬停的范围。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 复选框。 */
class Checkbox : public Widget {
 public:
  /** @param label 选项文字（UTF-8）。 */
  explicit Checkbox(std::string label);

  /** 设置文字。 */
  void setLabel(std::string label);
  /** 设置勾选状态，不触发 onChange（和网页版受控模式下由使用方设置值一致）。 */
  void setChecked(bool checked);
  [[nodiscard]] bool checked() const { return checked_; }
  /** 用户切换后回调新的勾选状态；回调之前控件已经切换。 */
  void setOnChange(std::function<void(bool)> handler) { onChange_ = std::move(handler); }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return enabled() ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;

 private:
  [[nodiscard]] TextStyle textStyle(Color color) const;
  void toggle();

  std::string label_;
  bool checked_{false};
  std::function<void(bool)> onChange_;
};

}  // namespace rujian::ui
