/*
 * ------------------------------------------------------------------------
 * 名称：row-list.hpp
 * 说明：信息行列表：每行是“前导文字、主文字、若干信息列、行尾操作”排成一行，鼠标移上去与按下时底色变化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 RowList 与 RowListItem，只做 Ember 用到的部分（前导、标题、信息列、操作；行选中与副文字没用到，不做）。
 *       文字单行显示，放不下时用省略号；组件库里按容器宽度隐藏“可选列”的规则没用到，不做。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/layout.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 信息行列表：行纵向排列、没有间距。 */
class RowList final : public FlexBox {
 public:
  RowList();
};

/** 信息行里的一列。 */
struct RowCell {
  /** 文字。 */
  std::string text;
  /** 固定宽度（逻辑像素）；0 表示取文字的宽度。 */
  double width{0};
  /** 是否用弱化色。 */
  bool muted{false};
};

/** 信息行。 */
class RowListItem : public Widget {
 public:
  RowListItem();

  /** 前导文字，占固定宽度；不设置就没有前导列。 */
  void setLeading(std::string leading);
  /** 主文字，占满剩余宽度。 */
  void setTitle(std::string title);
  /** 在主文字右边加一列。 */
  void addCell(RowCell cell);
  /** 行尾的操作（一般是 Actions 里放按钮）。 */
  void setActions(std::unique_ptr<Widget> actions);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  /** 按下时底色变深（行本身不响应点击）。 */
  bool onMouseDown(const MouseEvent& event) override;

 private:
  /** 按可用宽度算出的布局。 */
  struct Layout {
    double height{0};
    double leadingWidth{0};
    double mainWidth{0};
    std::vector<double> cellWidths;
    double actionsWidth{0};
  };

  [[nodiscard]] Layout layout(double width);
  /** 内容区（去掉上下内边距）里各列自己的高度的最大值。 */
  [[nodiscard]] double contentHeight(const Layout& laid);

  Label* leading_{nullptr};
  Label* title_{nullptr};
  std::vector<Label*> cells_;
  std::vector<RowCell> cellSpecs_;
  Widget* actions_{nullptr};
  bool hasLeading_{false};
};

}  // namespace rujian::ui
