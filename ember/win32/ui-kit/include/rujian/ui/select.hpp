/*
 * ------------------------------------------------------------------------
 * 名称：select.hpp
 * 说明：下拉框控件，对应网页版组件库的 Select：一个显示当前选项的触发器，点击或按键后在下方（放不下时在上方）弹出选项列表。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现 Ember 界面用到的属性（options、value、onChange、placeholder、allowEmpty）；手动输入、选项说明与预览图、无边框变体不做。
 *       弹层显示在窗口的弹层上（Host::showLayer），键盘焦点始终留在触发器上；选项多到放不下时可以用滚轮或方向键滚动，
 *       滚动条等阶段 3 的滚动区域做好后换成它；网页版弹层背后的背景虚化（backdrop-filter）不做，记入已知差异。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 下拉框的一个选项。 */
struct SelectOption {
  /** 选项的值（UTF-8），同一个下拉框里不重复。 */
  std::string value;
  /** 显示的文字（UTF-8）。 */
  std::string label;
};

class SelectPopup;

/** 下拉框。 */
class Select : public Widget {
 public:
  Select();
  ~Select() override;

  /** 设置选项。 */
  void setOptions(std::vector<SelectOption> options);
  /** 设置当前值，不触发 onChange；不在选项里的值显示成占位文字。 */
  void setValue(const std::string& value);
  [[nodiscard]] const std::string& value() const { return current_; }
  /** 没有选中项时显示的文字，默认“请选择”。 */
  void setPlaceholder(std::string placeholder);
  /** 是否在列表最前面放一个占位项用来清空，默认是。 */
  void setAllowEmpty(bool allowEmpty);
  /** 用户选了另一项后回调新的值；回调之前控件已经更新。 */
  void setOnChange(std::function<void(const std::string&)> handler) { onChange_ = std::move(handler); }

  /** 弹层是否展开。 */
  [[nodiscard]] bool open() const { return popup_ != nullptr; }
  /** 展开时第 index 个选项的区域（窗口坐标，含滚动偏移）；没有展开或序号超出范围时为空。画廊与测试用它定位选项。 */
  [[nodiscard]] std::optional<Rect> optionRect(std::size_t index) const;

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return enabled() ? CursorKind::Hand : CursorKind::Arrow; }
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  friend class SelectPopup;

  /** 弹层里的一项：占位项或选项。 */
  struct Entry {
    std::string value;
    std::string label;
    bool placeholder{false};
  };

  [[nodiscard]] std::vector<Entry> buildEntries() const;
  /** 当前值在选项里的位置（不含占位项），不在为 -1。 */
  [[nodiscard]] int optionIndexOf(const std::string& value) const;
  /** 触发器里显示的文字与它是不是占位文字。 */
  [[nodiscard]] const std::string& shownText(bool& isPlaceholder) const;
  [[nodiscard]] TextStyle textStyle(Color color) const;
  void openPopup(bool byKeyboard);
  void closePopup();
  /** 选中弹层里的第 index 项并关闭弹层。 */
  void choose(std::size_t index);

  std::vector<SelectOption> options_;
  std::string current_;
  std::string placeholder_{"请选择"};
  bool allowEmpty_{true};
  std::function<void(const std::string&)> onChange_;
  std::unique_ptr<SelectPopup> popup_;
  bool wasFocused_{false};
};

}  // namespace rujian::ui
