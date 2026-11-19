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
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/canvas.hpp"
#include "rujian/ui/geometry.hpp"
#include "rujian/ui/host.hpp"
#include "rujian/ui/tooltip-content.hpp"

namespace rujian::ui {

class Root;

/** 键盘修饰键。 */
enum Modifier : unsigned { kModifierShift = 1, kModifierCtrl = 2, kModifierAlt = 4 };

/** 鼠标按键。 */
enum class MouseButton { None, Left, Right, Middle };

/** 鼠标光标样式。 */
enum class CursorKind { Arrow, IBeam, Hand, SizeWE };

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
  /** 连续点击的次数（按下事件有效），1 为单击，2 为双击，3 为三连击。 */
  int clickCount{1};
};

/** 输入法的组合串。 */
struct ImeComposition {
  /** 正在组合的文字。 */
  std::u16string text;
  /** 组合串里的光标位置（UTF-16 单元）。 */
  std::size_t cursor{0};
  /** 输入法当前正在转换的分句（起始与结束，结束不含），没有时两个值相等。 */
  std::size_t targetStart{0};
  std::size_t targetEnd{0};
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
  /** 悬停在控件上时的鼠标光标样式。 */
  [[nodiscard]] virtual CursorKind cursor() const { return CursorKind::Arrow; }
  /** 是否需要输入法（有文字输入能力）；获得焦点时窗口据此开关输入法。 */
  [[nodiscard]] virtual bool acceptsText() const { return false; }
  /** 文字光标所在的矩形（窗口坐标），输入法的候选窗贴着它显示；不支持文字输入时为空。 */
  [[nodiscard]] virtual std::optional<Rect> caretRect() const { return std::nullopt; }
  /** 能否获得键盘焦点（Tab 键遍历与鼠标点击）。 */
  [[nodiscard]] virtual bool focusable() const { return false; }
  /** 焦点所在的区域（窗口坐标），滚动区域据此把它滚进可见范围；默认是控件自己，整个控件里自己管理焦点项的（如树）返回焦点项。 */
  [[nodiscard]] virtual Rect focusRect() const { return bounds(); }
  /** 在弹性布局的交叉轴上是否接受容器给的宽度（有固定宽度的控件返回 false）。 */
  [[nodiscard]] virtual bool stretchesWidth() const { return true; }
  /** 在弹性布局的交叉轴上是否接受容器给的高度（有固定高度的控件返回 false）。 */
  [[nodiscard]] virtual bool stretchesHeight() const { return true; }
  /** 请求重绘。 */
  void invalidate();

  // ---- 悬停提示 ----

  /** 设置纯文字的悬停提示；空文字表示这里没有提示，并盖住外层控件的提示。 */
  void setTooltip(std::string text);
  /** 设置版式的悬停提示。 */
  void setRichTooltip(TooltipContent content);
  /** 去掉自己的悬停提示（外层控件的提示重新生效）。 */
  void clearTooltip() { tooltip_.reset(); }
  /** 自己的悬停提示；没有设置时为空。 */
  [[nodiscard]] const std::optional<TooltipContent>& tooltip() const { return tooltip_; }

  // ---- 绘制 ----

  /** 绘制控件自己与后代：先 paint，再 paintChildren。 */
  void paintTree(Canvas& canvas);
  /** 绘制控件自己，默认什么也不画。 */
  virtual void paint(Canvas& canvas);
  /** 绘制子控件，默认按加入顺序逐个绘制；需要裁剪的容器重写它。 */
  virtual void paintChildren(Canvas& canvas);

  // ---- 命中 ----

  /** 找出点所在的最深处的可见控件，没有则为空。 */
  [[nodiscard]] virtual Widget* hitTest(Point point);
  /**
   * 作为弹层显示时实际盖住的区域（窗口坐标，追加到 out）：默认是控件自己；铺满窗口但只有一部分可见的弹层（通知、提示）重写它。
   * 应用据此把压在窗口上的原生子窗口（如运行画面）挖掉对应区域，弹层才能显示在它之上。
   */
  virtual void layerRects(std::vector<Rect>& out) const;

  // ---- 事件处理，返回 true 表示已处理 ----

  virtual bool onMouseDown(const MouseEvent& event);
  virtual bool onMouseUp(const MouseEvent& event);
  virtual bool onMouseMove(const MouseEvent& event);
  virtual bool onMouseWheel(const MouseEvent& event);
  virtual bool onKeyDown(const KeyEvent& event);
  virtual bool onKeyUp(const KeyEvent& event);
  virtual bool onChar(char32_t codePoint);
  /** 输入法组合串变化（仅 acceptsText 的控件收到）。 */
  virtual void onImeComposition(const ImeComposition& composition);
  /** 输入法确认了一段文字（仅 acceptsText 的控件收到）。 */
  virtual void onImeCommit(std::u16string_view text);
  /** 输入法组合结束（确认或取消后都会收到）。 */
  virtual void onImeEnd();
  /** 悬停、按下、焦点等状态变化后调用，默认重绘。 */
  virtual void onStateChanged();
  /** Host::startTimer 的定时器到时间（一次性）。 */
  virtual void onTimer(unsigned id);
  /** 某个后代控件获得了键盘焦点（滚动区域据此把它滚到可见处）。 */
  virtual void onDescendantFocused(Widget* descendant);

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
  std::optional<TooltipContent> tooltip_;
};

}  // namespace rujian::ui
