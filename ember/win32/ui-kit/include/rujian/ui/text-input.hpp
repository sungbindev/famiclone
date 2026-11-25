/*
 * ------------------------------------------------------------------------
 * 名称：text-input.hpp
 * 说明：单行文字输入框，对应网页版组件库的 TextInput：自己实现编辑（光标、选区、复制粘贴、撤销与重做、按词移动与删除、鼠标选择、输入法组合与候选窗位置）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现 Ember 界面用到的属性（类型 search、占位文字、校验失败、值、值变化、失去焦点、回车）；前缀文字与密码显示切换没有用到，不做。
 *       “回车”在输入法组合中是确认候选词，不会触发回车回调；值变化回调只在用户编辑时触发，外部 setValue 不触发。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class LineEditor;

/** 单行文字输入框。 */
class TextInput : public Widget {
 public:
  TextInput();
  ~TextInput() override;

  /** 设置值（UTF-8）；与当前值相同时不动光标，不同时光标移到末尾并清空撤销历史。 */
  void setValue(const std::string& value);
  /** 当前值（UTF-8）。 */
  [[nodiscard]] std::string value() const;
  /** 选中值里 [start, end) 这一段，位置是 UTF-8 的字节偏移（超出的部分截到末尾）。 */
  void selectRange(std::size_t start, std::size_t end);
  /** 选中文件名里扩展名之前的部分（最后一个点之前；没有点或点在开头时选中全部）。 */
  void selectBaseName();
  /** 占位文字。 */
  void setPlaceholder(std::string placeholder);
  /** 校验失败：边框与焦点环变成错误色。 */
  void setInvalid(bool invalid);
  /** 搜索类型：有文字且鼠标悬停或有焦点时在右侧显示清除按钮。 */
  void setSearch(bool search);
  /** 用户编辑后值变化的回调，参数是新值。 */
  void setOnChange(std::function<void(const std::string&)> handler) { onChange_ = std::move(handler); }
  /** 失去键盘焦点的回调。 */
  void setOnBlur(std::function<void()> handler) { onBlur_ = std::move(handler); }
  /** 按回车的回调（输入法组合中的回车不触发）。 */
  void setOnEnter(std::function<void()> handler) { onEnter_ = std::move(handler); }

  [[nodiscard]] Size measure(Size available) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  [[nodiscard]] CursorKind cursor() const override { return enabled() ? CursorKind::IBeam : CursorKind::Arrow; }
  [[nodiscard]] bool acceptsText() const override { return true; }
  [[nodiscard]] std::optional<Rect> caretRect() const override;
  void paint(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onChar(char32_t codePoint) override;
  void onImeComposition(const ImeComposition& composition) override;
  void onImeCommit(std::u16string_view text) override;
  void onImeEnd() override;
  void onStateChanged() override;

 private:
  /** 输入区（文字可见的范围，窗口坐标）。 */
  [[nodiscard]] Rect contentRect() const;
  [[nodiscard]] TextStyle textStyle(Color color) const;
  /** 屏幕上显示的文字：正在组合时把组合串插在光标处。 */
  [[nodiscard]] std::u16string displayText() const;
  /** 光标在 displayText 里的位置（组合时在组合串里的光标处）。 */
  [[nodiscard]] std::size_t displayCaret() const;
  /** 窗口坐标的 x 对应的 value 里的位置。 */
  [[nodiscard]] std::size_t indexAt(double x) const;
  /** 编辑后同步：滚动到光标可见、重新开始光标闪烁、触发值变化回调。 */
  void afterEdit(bool changed);
  void ensureCaretVisible();
  /** 清除按钮（搜索类型）的区域；不显示时为空。 */
  [[nodiscard]] std::optional<Rect> clearButtonRect() const;
  void notifyChanged();

  std::unique_ptr<LineEditor> editor_;
  std::string placeholder_;
  bool invalid_{false};
  bool search_{false};
  std::function<void(const std::string&)> onChange_;
  std::function<void()> onBlur_;
  std::function<void()> onEnter_;
  /** 文字相对输入区左边缘向左滚动的距离。 */
  double scroll_{0};
  bool dragging_{false};
  bool wasFocused_{false};
  /** 输入法组合串；为空表示没有在组合。 */
  ImeComposition composition_;
  bool composing_{false};
};

}  // namespace rujian::ui
