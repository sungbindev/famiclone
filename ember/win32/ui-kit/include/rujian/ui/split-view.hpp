/*
 * ------------------------------------------------------------------------
 * 名称：split-view.hpp
 * 说明：分栏视图：左边一栏（可滚动）、右边主区，中间的分隔条可以拖动或用键盘调整左栏宽度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 SplitView，只做 Ember 用到的部分（左栏内容、主区内容、初始宽度、最小宽度、最大宽度）；
 *       左栏表头（标题与右侧操作）暂未支持。左栏右边有一条分割线，分隔条（宽 4）跨在这条线两侧，鼠标移上去停留 200 毫秒后显示聚焦色。
 *       高度由摆放它的区域决定；度量时取左栏与主区中较高的。
 *       主区可以隐藏：隐藏后左栏占满整个宽度，分割线与分隔条都不出现；左栏可以设成铺满（内容按左栏的高度铺满、不滚动，内容自己带滚动区域时用）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class ScrollArea;
class SplitterHandle;

/** 分栏视图。 */
class SplitView : public Widget {
 public:
  SplitView();

  /** 左栏的内容（放在滚动区域里）。 */
  void setSide(std::unique_ptr<Widget> side);
  /** 主区的内容，占满主区。 */
  void setMain(std::unique_ptr<Widget> main);
  /** 左栏宽度，默认 220，会被限制在最小与最大宽度之间。 */
  void setSideWidth(double width);
  [[nodiscard]] double sideWidth() const { return sideWidth_; }
  /** 主区左边让出的宽度：左栏加它与主区之间的间距。 */
  [[nodiscard]] double sideExtent() const;
  /** 分隔条（给测试与对照用，它是可获得键盘焦点的控件）。 */
  [[nodiscard]] Widget* splitter() const;
  /** 左栏宽度的范围，默认 160 到 480。 */
  void setSideWidthRange(double minimum, double maximum);
  /** 左栏内容铺满左栏的高度、不滚动（默认关闭）。 */
  void setSideFit(bool fit);
  /** 显示或隐藏主区（默认显示）；隐藏时左栏占满整个宽度，分割线与分隔条都不出现。 */
  void setMainVisible(bool visible);
  [[nodiscard]] bool mainVisible() const { return mainVisible_; }

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;

 private:
  friend class SplitterHandle;

  /** 把宽度限制在范围内并取整。 */
  [[nodiscard]] double clampWidth(double width) const;
  /** 分隔条改变宽度后重新摆放。 */
  void changeSideWidth(double width);

  ScrollArea* sideScroll_{nullptr};
  Widget* main_{nullptr};
  SplitterHandle* splitter_{nullptr};
  double sideWidth_{220.0};
  double minimum_{160.0};
  double maximum_{480.0};
  bool mainVisible_{true};
};

}  // namespace rujian::ui
