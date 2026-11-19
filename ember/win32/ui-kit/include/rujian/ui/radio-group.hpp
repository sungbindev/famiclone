/*
 * ------------------------------------------------------------------------
 * 名称：radio-group.hpp
 * 说明：单选组控件，对应网页版组件库的 RadioGroup 的分段变体（segmented）：选项等宽排成一行，选中项有底色和下方强调线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：Ember 只用分段变体，竖排、横排、选项说明不做；选项的悬停提示由外壳类控件阶段的提示控件统一处理，这里不显示。
 *       整组只有一个 Tab 停靠点（已选项，没有选中时是第一个可用项），方向键在可用选项间循环移动并选中。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 单选组的一个选项。 */
struct RadioOption {
  /** 选项的值（UTF-8），同一组里不重复。 */
  std::string value;
  /** 显示的文字（UTF-8）。 */
  std::string label;
  /** 这一项是否禁用。 */
  bool disabled{false};
};

/** 分段单选组。 */
class RadioGroup : public Widget {
 public:
  RadioGroup() = default;

  /** 设置选项。 */
  void setOptions(std::vector<RadioOption> options);
  /** 设置选中的值，不触发 onChange；空串或不存在的值表示都不选。 */
  void setValue(const std::string& value);
  /** 当前选中的值，都不选时为空串。 */
  [[nodiscard]] std::string value() const;
  /** 用户选中另一项后回调新的值；回调之前控件已经切换。 */
  void setOnChange(std::function<void(const std::string&)> handler) { onChange_ = std::move(handler); }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override;
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override;
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  /** 第 index 个选项占的区域（含它左边的分隔线）。 */
  [[nodiscard]] Rect itemRect(std::size_t index) const;
  /** 容器与分隔线的边框宽度：1px 取整到设备像素（至少 1 个设备像素），网页版的布局也是这样算的。 */
  [[nodiscard]] double borderWidth() const;
  /** 点所在的选项序号，不在任何选项上时为 -1。 */
  [[nodiscard]] int indexAt(Point point) const;
  [[nodiscard]] bool itemEnabled(int index) const;
  /** 从 from 起按方向循环找下一个可用选项，都不可用时为 -1。 */
  [[nodiscard]] int nextEnabled(int from, int step) const;
  [[nodiscard]] int firstEnabled() const { return nextEnabled(-1, 1); }
  [[nodiscard]] TextStyle textStyle(bool selected, Color color) const;
  void select(int index);

  std::vector<RadioOption> options_;
  int selected_{-1};
  /** 键盘焦点所在的选项（有焦点时才有意义）。 */
  int focusIndex_{-1};
  int hoverIndex_{-1};
  int pressIndex_{-1};
  bool wasFocused_{false};
  std::function<void(const std::string&)> onChange_;
};

}  // namespace rujian::ui
