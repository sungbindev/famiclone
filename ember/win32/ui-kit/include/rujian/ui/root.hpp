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
#include <memory>
#include <vector>

#include "rujian/ui/host.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

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

  void setTheme(ThemeName theme);
  void setScale(double scale);

  /** 布局：让内容铺满指定的窗口大小（逻辑像素）。 */
  void layout(Size size);
  [[nodiscard]] Size size() const { return size_; }

  /** 绘制整棵控件树。 */
  void paint(Canvas& canvas);

  // ---- 输入事件，坐标是窗口坐标 ----

  void mouseMove(Point position, unsigned modifiers);
  void mouseDown(Point position, MouseButton button, unsigned modifiers);
  void mouseUp(Point position, MouseButton button, unsigned modifiers);
  void mouseWheel(Point position, double delta, unsigned modifiers);
  /** 鼠标离开窗口：清除悬停。 */
  void mouseLeave();
  void keyDown(int virtualKey, unsigned modifiers, bool repeat);
  void keyUp(int virtualKey, unsigned modifiers);
  void character(char32_t codePoint);

  // ---- Host ----

  [[nodiscard]] ThemeName theme() const override { return theme_; }
  [[nodiscard]] double scale() const override { return scale_; }
  void invalidate() override;
  [[nodiscard]] TextMetrics measureText(std::string_view text, const TextStyle& style, double maxWidth) override;
  [[nodiscard]] Widget* focusedWidget() const override { return focus_; }
  void setFocus(Widget* widget, bool visible) override;
  void captureMouse(Widget* widget) override { capture_ = widget; }
  void releaseMouse(Widget* widget) override {
    if (capture_ == widget) capture_ = nullptr;
  }
  void widgetLeft(Widget* widget) override;

 private:
  void updateHover(Widget* target);
  void moveFocus(bool backward);
  static void collectFocusable(Widget* widget, std::vector<Widget*>& out);

  ThemeName theme_;
  double scale_;
  Size size_{};
  std::function<void()> invalidateHandler_;
  Widget* focus_{nullptr};
  Widget* hover_{nullptr};
  Widget* pressedWidget_{nullptr};
  Widget* capture_{nullptr};
  // 内容放最后声明，保证它最先销毁，销毁时上面的指针还有效
  std::unique_ptr<Widget> content_;
};

}  // namespace rujian::ui
