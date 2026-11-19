/*
 * ------------------------------------------------------------------------
 * 名称：scroll-area.hpp
 * 说明：滚动区域控件，对应网页版组件库的 ScrollArea：纵向滚动、滚动条盖在内容上，鼠标移入或滚动时显示，停下后渐隐。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只实现 Ember 用到的属性（horizontal=hidden，即只有纵向滚动）；bleed 留到用到它的容器（阶段 5 的边栏与停靠面板）再做。
 *       滚动条的尺寸与行为取自 VS Code 的滚动条：轨道宽 10、滑块宽 6 居中、最短 20；在滑块上按下拖动，在轨道上按下滑块先跳到指针处再拖动；
 *       显示渐变 100 毫秒、隐藏渐变 800 毫秒，停止滚动或鼠标移出后 500 毫秒才开始隐藏，宿主关闭动画时直接切换。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 滚动区域。 */
class ScrollArea : public Widget {
 public:
  ScrollArea() = default;

  /** 设置被滚动的内容（唯一的子控件），替换原来的。 */
  void setContent(std::unique_ptr<Widget> content);
  [[nodiscard]] Widget* content() const { return content_; }
  /** 自然高度的上限（0 表示不限）：内容比它高时，度量出的高度停在上限，超出的部分滚动。 */
  void setMaxHeight(double maxHeight);

  /** 当前滚动位置（内容顶部被卷上去的距离）。 */
  [[nodiscard]] double scrollTop() const { return scrollTop_; }
  /** 设置滚动位置，限制在 0 到最大滚动距离之间；位置变化时显示滚动条。 */
  void setScrollTop(double top);
  /** 内容的总高度。 */
  [[nodiscard]] double contentHeight() const { return contentHeight_; }
  /** 最大滚动距离。 */
  [[nodiscard]] double maxScrollTop() const;
  /** 滚动到让窗口坐标里的这块区域完全可见（已经可见时不动）。 */
  void scrollIntoView(const Rect& area);

  /** 滚动条此刻的不透明度，0 到 1。 */
  [[nodiscard]] double scrollbarOpacity() const { return opacity_; }
  /** 滚动条滑块的区域（窗口坐标）；内容放得下、没有滚动条时为空区域。 */
  [[nodiscard]] Rect sliderRect() const;

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  Widget* hitTest(Point point) override;
  void paintChildren(Canvas& canvas) override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onMouseWheel(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;
  void onTimer(unsigned id) override;
  void onDescendantFocused(Widget* descendant) override;

 private:
  /** 轨道（可点击拖动的整条）的区域。 */
  [[nodiscard]] Rect trackRect() const;
  [[nodiscard]] bool scrollbarNeeded() const { return maxScrollTop() > 0; }
  /** 滑块的长度与位置（相对轨道顶部）。 */
  [[nodiscard]] double sliderLength() const;
  [[nodiscard]] double sliderOffset() const;
  /** 重新摆放内容并重绘。 */
  void applyScroll();
  /** 根据鼠标、滚动、拖动重新决定滚动条是否应该显示。 */
  void updateVisibility();
  void setTarget(bool visible);
  void scheduleHide();
  void tickAnimation();

  Widget* content_{nullptr};
  double maxHeight_{0};
  double contentHeight_{0};
  double scrollTop_{0};
  double opacity_{0};
  bool targetVisible_{false};
  /** 最近滚动过（还没到隐藏的时间）。 */
  bool recentlyScrolled_{false};
  bool dragging_{false};
  /** 拖动开始时指针到滑块顶部的距离。 */
  double dragGrab_{0};
  bool overSlider_{false};
  unsigned hideTimer_{0};
  unsigned animationTimer_{0};
};

}  // namespace rujian::ui
