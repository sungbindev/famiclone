/*
 * ------------------------------------------------------------------------
 * 名称：widget.hpp
 * 说明：控件基类：控件树、布局（度量与摆放）、绘制、状态（悬停、按下、焦点、可用、可见）与输入事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：控件的 bounds 是窗口坐标（逻辑像素）；事件处理函数返回 true 表示已处理，否则事件向父控件冒泡；
 *       控件的颜色在绘制时按宿主当前主题现取，不缓存，切换主题只需重绘。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <vector>

#include "rujian/ui/canvas.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/host.hpp"

namespace rujian::ui {

class Root;

/** 键盘修饰键。 */
enum Modifier : unsigned { kModifierShift = 1, kModifierCtrl = 2, kModifierAlt = 4 };

/** 鼠标按键。 */
enum class MouseButton { None, Left, Right, Middle };

/** 鼠标事件。 */
struct MouseEvent {
  /** 位置，窗口坐标。 */
  Point position;
  /** 触发事件的按键；移动与滚轮为 None。 */
  MouseButton button{MouseButton::None};
  /** 修饰键组合。 */
  unsigned modifiers{0};
  /** 滚轮滚动量，一格为 1，向上为正。 */
  double wheelDelta{0};
};

/** 键盘事件。 */
struct KeyEvent {
  /** Windows 虚拟键码。 */
  int virtualKey{0};
  /** 修饰键组合。 */
  unsigned modifiers{0};
  /** 是否是按住不放产生的重复事件。 */
  bool repeat{false};
};

/** 控件基类。 */
class Widget {
 public:
  Widget() = default;
  virtual ~Widget();
  Widget(const Widget&) = delete;
  Widget& operator=(const Widget&) = delete;

  // ---- 控件树 ----

  /** 加入子控件，返回子控件的指针。 */
  Widget* addChild(std::unique_ptr<Widget> child);
  /** 移除子控件并交还所有权；不是自己的子控件时返回空。 */
  std::unique_ptr<Widget> removeChild(Widget* child);
  [[nodiscard]] Widget* parent() const { return parent_; }
  [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }
  /** 宿主；控件树还没挂到 Root 上时为空。 */
  [[nodiscard]] Host* host() const { return host_; }

  // ---- 布局 ----

  /**
   * 度量：返回控件想要的尺寸。
   * @param available 可用空间的上限，宽或高为 0 表示不限制。
   */
  [[nodiscard]] virtual Size measure(Size available);
  /** 摆放：设置 bounds 并摆放子控件；默认让每个子控件铺满自己。 */
  virtual void arrange(const Rect& bounds);
  /** 控件占据的区域，窗口坐标。 */
  [[nodiscard]] const Rect& bounds() const { return bounds_; }

  // ---- 状态 ----

  [[nodiscard]] bool visible() const { return visible_; }
  void setVisible(bool visible);
  [[nodiscard]] bool enabled() const { return enabled_; }
  void setEnabled(bool enabled);
  /** 鼠标是否在控件或其后代上方。 */
  [[nodiscard]] bool hovered() const { return hovered_; }
  /** 鼠标左键是否按在控件上。 */
  [[nodiscard]] bool pressed() const { return pressed_; }
  /** 是否有键盘焦点。 */
  [[nodiscard]] bool focused() const { return focused_; }
  /** 是否要显示焦点环（键盘切换到的焦点，对应 CSS 的 :focus-visible）。 */
  [[nodiscard]] bool focusVisible() const { return focused_ && focusVisible_; }
  /** 能否获得键盘焦点（Tab 键遍历与鼠标点击）。 */
  [[nodiscard]] virtual bool focusable() const { return false; }
  /** 请求重绘。 */
  void invalidate();

  // ---- 绘制 ----

  /** 绘制控件自己与后代：先 paint，再 paintChildren。 */
  void paintTree(Canvas& canvas);
  /** 绘制控件自己，默认什么也不画。 */
  virtual void paint(Canvas& canvas);
  /** 绘制子控件，默认按加入顺序逐个绘制；需要裁剪的容器重写它。 */
  virtual void paintChildren(Canvas& canvas);

  // ---- 命中 ----

  /** 找出点所在的最深处的可见控件，没有则为空。 */
  [[nodiscard]] Widget* hitTest(Point point);

  // ---- 事件处理，返回 true 表示已处理 ----

  virtual bool onMouseDown(const MouseEvent& event);
  virtual bool onMouseUp(const MouseEvent& event);
  virtual bool onMouseMove(const MouseEvent& event);
  virtual bool onMouseWheel(const MouseEvent& event);
  virtual bool onKeyDown(const KeyEvent& event);
  virtual bool onKeyUp(const KeyEvent& event);
  virtual bool onChar(char32_t codePoint);
  /** 悬停、按下、焦点等状态变化后调用，默认重绘。 */
  virtual void onStateChanged();

 private:
  friend class Root;

  void attach(Host* host);
  void detach();

  Widget* parent_{nullptr};
  Host* host_{nullptr};
  std::vector<std::unique_ptr<Widget>> children_;
  Rect bounds_{};
  bool visible_{true};
  bool enabled_{true};
  bool hovered_{false};
  bool pressed_{false};
  bool focused_{false};
  bool focusVisible_{false};
};

}  // namespace rujian::ui
