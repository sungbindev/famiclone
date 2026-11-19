/*
 * ------------------------------------------------------------------------
 * 名称：root.hpp
 * 说明：根容器：持有一棵控件树，实现宿主接口，负责布局、绘制、命中、悬停、按下、焦点遍历与输入事件分发。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口（HostWindow）与离屏绘制（OffscreenSurface）都用 Root；输入事件的坐标是窗口坐标（逻辑像素）。
 *       Tab 与 Shift+Tab 在 Root 里处理：按控件树的先后顺序在可用、可见、可获得焦点的控件之间切换。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rujian/ui/host.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 悬停提示的显示者（由 Tooltip 控件实现）：接收 Root 决定显示或收起提示的请求。 */
class TooltipPresenter {
 public:
  virtual ~TooltipPresenter() = default;
  /** 显示 target 的提示；pointer 是鼠标位置（窗口坐标）。 */
  virtual void showTooltip(const Widget& target, const TooltipContent& content, Point pointer) = 0;
  /** 收起提示。 */
  virtual void hideTooltip() = 0;
};

/** 根容器。 */
class Root final : public Host {
 public:
  /**
   * @param theme 初始主题。
   * @param scale 初始缩放比例，必须大于 0。
   */
  Root(ThemeName theme, double scale);
  ~Root() override;
  Root(const Root&) = delete;
  Root& operator=(const Root&) = delete;

  /** 设置内容控件（整棵控件树的根），替换原来的内容。 */
  void setContent(std::unique_ptr<Widget> content);
  [[nodiscard]] Widget* content() const { return content_.get(); }

  /** 设置重绘请求的回调，窗口借此调用 InvalidateRect。 */
  void setInvalidateHandler(std::function<void()> handler) { invalidateHandler_ = std::move(handler); }

  /**
   * 设置弹层盖住的区域变化时的回调（每次绘制后比较）：参数是全部弹层此刻盖住的区域（窗口坐标）；
   * 窗口借此把压在控件之上的原生子窗口挖掉对应的区域。
   */
  void setLayerRectsHandler(std::function<void(const std::vector<Rect>&)> handler) { layerRectsHandler_ = std::move(handler); }

  /**
   * 设置剪贴板的读写函数（UTF-8）；不设置时用系统剪贴板，测试里用内存里的剪贴板代替。
   * @param read 读；没有文字时返回空值。
   * @param write 写。
   */
  void setClipboardHandlers(std::function<std::optional<std::string>()> read, std::function<void(std::string_view)> write);

  /** 设置悬停提示的显示者；空表示不显示提示。不负责显示者的生命周期。 */
  void setTooltipPresenter(TooltipPresenter* presenter);
  /** 收起悬停提示并取消等待中的提示（窗口失去焦点、窗口大小变化时调用）。 */
  void hideTooltip();
  /** 鼠标停在控件上多久后弹出提示（毫秒）。 */
  static constexpr unsigned kTooltipDelayMilliseconds = 400;

  /**
   * 设置光标闪烁定时器的回调：参数是间隔毫秒，0 表示停止。窗口据此设置系统定时器，到时间后调用 toggleCaret。
   */
  void setCaretTimerHandler(std::function<void(unsigned)> handler) { caretTimerHandler_ = std::move(handler); }

  /** 光标闪烁定时器到时间：切换光标的显示相位并重绘。 */
  void toggleCaret();

  /** 把文字光标一直隐藏（离屏截图对照时用，让截图不随闪烁相位变化）。 */
  void setCaretHidden(bool hidden) { caretHidden_ = hidden; }

  /** 设置连续点击的判定：两次点击的间隔不超过 timeMs、位置相距不超过 slop（逻辑像素）。 */
  void setDoubleClick(unsigned timeMs, double slop) {
    doubleClickMs_ = timeMs;
    doubleClickSlop_ = slop;
  }

  void setTheme(ThemeName theme);
  void setScale(double scale);

  /** 布局：让内容铺满指定的窗口大小（逻辑像素）。 */
  void layout(Size size);
  [[nodiscard]] Size size() const { return size_; }

  /** 按“在弹层之外点了一下”通知全部弹层（菜单等会据此关闭）；窗口的非客户区被点击、窗口失去激活时调用，这些情况不会有客户区的鼠标消息。 */
  void dismissLayers();

  /** 绘制整棵控件树。 */
  void paint(Canvas& canvas);

  // ---- 输入事件，坐标是窗口坐标 ----

  void mouseMove(Point position, unsigned modifiers);
  /** @param timeMs 事件时间（毫秒），用来判断连续点击；不关心连续点击时传 0。 */
  void mouseDown(Point position, MouseButton button, unsigned modifiers, unsigned timeMs = 0);
  void mouseUp(Point position, MouseButton button, unsigned modifiers);
  void mouseWheel(Point position, double delta, unsigned modifiers);
  /** 鼠标离开窗口：清除悬停。 */
  void mouseLeave();
  void keyDown(int virtualKey, unsigned modifiers, bool repeat);
  void keyUp(int virtualKey, unsigned modifiers);
  void character(char32_t codePoint);

  // ---- 输入法（只发给有键盘焦点且需要文字输入的控件）----

  /** 输入法组合串变化。 */
  void imeComposition(const ImeComposition& composition);
  /** 输入法确认了一段文字。 */
  void imeCommit(std::u16string_view text);
  /** 输入法组合结束。 */
  void imeEnd();
  /** 输入法候选窗应贴着显示的光标矩形（窗口坐标）；焦点控件不支持文字输入时为空。 */
  [[nodiscard]] std::optional<Rect> imeCaretRect() const;
  /** 当前焦点控件是否需要输入法。 */
  [[nodiscard]] bool focusAcceptsText() const;
  /** 鼠标此刻应显示的光标样式。 */
  [[nodiscard]] CursorKind cursor() const;

  // ---- Host ----

  [[nodiscard]] ThemeName theme() const override { return theme_; }
  [[nodiscard]] double scale() const override { return scale_; }
  void invalidate() override;
  void requestLayout() override { layout(size_); }
  [[nodiscard]] TextMetrics measureText(std::string_view text, const TextStyle& style, double maxWidth) override;
  [[nodiscard]] TextMetrics measureText16(std::u16string_view text, const TextStyle& style, double maxWidth) override;
  [[nodiscard]] double caretOffset(std::u16string_view text, const TextStyle& style, std::size_t index) override;
  [[nodiscard]] std::size_t caretIndexAt(std::u16string_view text, const TextStyle& style, double x) override;
  [[nodiscard]] std::optional<std::string> clipboardText() override;
  void setClipboardText(std::string_view text) override;
  [[nodiscard]] bool caretOn() const override { return !caretHidden_ && caretPhase_; }
  void restartCaretBlink() override;
  [[nodiscard]] Widget* focusedWidget() const override { return focus_; }
  void setFocus(Widget* widget, bool visible) override;
  void captureMouse(Widget* widget) override { capture_ = widget; }
  void releaseMouse(Widget* widget) override {
    if (capture_ == widget) capture_ = nullptr;
  }
  void widgetLeft(Widget* widget) override;
  [[nodiscard]] Size viewportSize() const override { return size_; }
  void showLayer(Widget* layer, std::function<void(Widget* hit)> onOutsidePress) override;
  void hideLayer(Widget* layer) override;
  unsigned startTimer(Widget* owner, unsigned milliseconds) override;
  void stopTimer(unsigned id) override;
  [[nodiscard]] bool animationsEnabled() const override { return animations_; }
  /** 开关动画，默认开。 */
  void setAnimationsEnabled(bool enabled) { animations_ = enabled; }

  /**
   * 设置系统定时器的启动与取消函数（窗口用 SetTimer、KillTimer）；不设置时定时器不会自己触发，测试用 fireTimer 手动触发。
   */
  void setTimerHandlers(std::function<void(unsigned id, unsigned milliseconds)> start, std::function<void(unsigned id)> stop);
  /** 系统定时器到时间：取消它并调用持有者的 onTimer。 */
  void fireTimer(unsigned id);
  /** 还没触发的定时器编号（测试用）。 */
  [[nodiscard]] std::vector<unsigned> pendingTimers() const;

 private:
  /** 显示中的一个弹层。 */
  struct Layer {
    Widget* widget;
    std::function<void(Widget* hit)> onOutsidePress;
  };

  /** 点所在的最深控件：先找弹层（从上到下），再找内容。 */
  Widget* hitTarget(Point point);
  /** 控件是否属于某个弹层。 */
  [[nodiscard]] bool inLayer(const Widget* widget) const;
  void updateHover(Widget* target);
  void updateTooltip(Widget* hit, Point position);
  void showTooltipNow();
  void dismissTooltip();
  void moveFocus(bool backward);
  static void collectFocusable(Widget* widget, std::vector<Widget*>& out);

  ThemeName theme_;
  double scale_;
  Size size_{};
  std::function<void()> invalidateHandler_;
  std::function<void(const std::vector<Rect>&)> layerRectsHandler_;
  std::vector<Rect> lastLayerRects_;
  Widget* focus_{nullptr};
  Widget* hover_{nullptr};
  Widget* pressedWidget_{nullptr};
  Widget* capture_{nullptr};
  std::function<std::optional<std::string>()> clipboardRead_;
  std::function<void(std::string_view)> clipboardWrite_;
  std::function<void(unsigned)> caretTimerHandler_;
  bool caretHidden_{false};
  bool caretPhase_{true};
  unsigned doubleClickMs_{500};
  double doubleClickSlop_{4.0};
  unsigned lastClickTime_{0};
  Point lastClickPosition_{};
  MouseButton lastClickButton_{MouseButton::None};
  int clickCount_{0};
  std::vector<Layer> layers_;
  std::map<unsigned, Widget*> timers_;
  unsigned nextTimer_{100};
  bool animations_{true};
  TooltipPresenter* tooltipPresenter_{nullptr};
  /** 等待显示或正在显示提示的控件。 */
  Widget* tooltipTarget_{nullptr};
  /** 提示被点击、按键、滚轮收起后，鼠标离开它之前不再弹出。 */
  Widget* tooltipBlocked_{nullptr};
  bool tooltipVisible_{false};
  unsigned tooltipTimer_{0};
  Point tooltipPointer_{};
  /** 提示延迟定时器的持有者。 */
  std::unique_ptr<Widget> tooltipTimerOwner_;
  std::function<void(unsigned, unsigned)> timerStart_;
  std::function<void(unsigned)> timerStop_;
  // 内容放最后声明，保证它最先销毁，销毁时上面的指针还有效
  std::unique_ptr<Widget> content_;
};

}  // namespace rujian::ui
