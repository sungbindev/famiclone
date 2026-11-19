/*
 * ------------------------------------------------------------------------
 * 名称：panel.hpp
 * 说明：面板：带边框的卡片，有标题或 aside 时带标题行，内容放进内容区，内容区在滚动区域里滚动。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Panel；内容区只有一列纵向堆叠（内边距、间距取面板令牌）。
 *       只传 aside 时标题行里只有 aside，靠左排列；内容区没有内容时整块收起，标题行下面也不画分割线。
 *       副标题、底部、占满剩余高度（grow）暂未支持。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;
class ScrollArea;
class Stack;

/** 面板。 */
class Panel : public Widget {
 public:
  Panel();

  /** 设置标题。 */
  void setTitle(std::string title);
  /** 设置标题行里的元素（进度、数量、操作按钮），有标题时在右侧；传空表示移除。 */
  void setAside(std::unique_ptr<Widget> aside);
  /** 紧凑标题栏：标题行高度与上下内边距取紧凑令牌。 */
  void setCompactHead(bool compactHead);
  /** 去掉卡片外观（底色、边框、圆角、阴影）。 */
  void setPlain(bool plain);
  /** 往内容区加入一项，返回它的指针。 */
  Widget* addContent(std::unique_ptr<Widget> content);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  void paintChildren(Canvas& canvas) override;

 private:
  [[nodiscard]] double borderWidth() const;
  [[nodiscard]] bool hasHead() const;
  [[nodiscard]] bool hasBody() const;
  /** 标题行的高度（含下边的分割线），没有标题行时为 0。 */
  [[nodiscard]] double headHeight(double innerWidth);

  Label* title_{nullptr};
  Widget* aside_{nullptr};
  ScrollArea* scroll_{nullptr};
  Stack* body_{nullptr};
  bool compactHead_{false};
  bool plain_{false};
};

}  // namespace rujian::ui
