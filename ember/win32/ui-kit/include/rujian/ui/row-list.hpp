/*
 * ------------------------------------------------------------------------
 * 名称：row-list.hpp
 * 说明：信息行列表：每行是“前导文字、主文字、若干信息列、行尾操作”排成一行，鼠标移上去时底色变化，按下时不变（行不是按钮）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 RowList 与 RowListItem，只做 Ember 用到的部分（前导、标题、信息列、操作；行选中与副文字没用到，不做）。
 *       文字单行显示，放不下时用省略号；组件库里按容器宽度隐藏“可选列”的规则没用到，不做。
 *       前导可以带图标、改宽度、用强调样式（强调色图标），信息列可以设为可点击（setCellAction），这些是本控件库自己的扩展，网页版没有。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/icon-id.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 信息行列表：行纵向排列、没有间距。 */
class RowList final : public FlexBox {
 public:
  RowList();
};

/** 信息行的前导列。 */
struct RowLeading {
  /** 文字。 */
  std::string text;
  /** 文字左边的图标；没有就不画。 */
  std::optional<IconId> icon;
  /** 固定宽度（逻辑像素，含图标）；0 表示取行列表令牌的前导宽度。 */
  double width{0};
  /** 是否强调：图标用强调色（文字仍是常规字重，微软雅黑没有半粗字重）。 */
  bool emphasized{false};
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
  /** 前导列，可以带图标、指定宽度与强调样式。 */
  void setLeading(RowLeading leading);
  /** 主文字，占满剩余宽度。 */
  void setTitle(std::string title);
  /** 在主文字右边加一列。 */
  void addCell(RowCell cell);
  /** 修改第 index 列的文字；列宽取文字宽度时同时重排这一行。 */
  void setCellText(std::size_t index, std::string text);
  /**
   * 让第 index 列可点击：鼠标移到列上时列的底色变化、光标变手形，在列上按下并抬起时回调，参数是这一列的点击区域（窗口坐标），弹层据此定位。
   * 这是本控件库自己的扩展，网页版没有；handler 为空表示取消。
   */
  void setCellAction(std::size_t index, std::function<void(const Rect&)> handler);
  /** 第 index 列的点击是否可用；不可用时没有悬停效果，点击也不回调。默认可用。 */
  void setCellActionEnabled(std::size_t index, bool enabled);
  /** 行尾的操作（一般是 Actions 里放按钮）。 */
  void setActions(std::unique_ptr<Widget> actions);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  /** 按下时行的底色不变（行不是按钮，也不响应点击）；可点击的列按下时列底色变深，在抬起时回调。 */
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  void onStateChanged() override;
  [[nodiscard]] CursorKind cursor() const override { return hoverCell_ >= 0 ? CursorKind::Hand : CursorKind::Arrow; }

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
  /** 可点击的列的点击区域（窗口坐标）：文字区域左右各多一点，上下贴着行。 */
  [[nodiscard]] Rect cellHitRect(std::size_t index) const;
  /** 点在哪个可点击的列上，没有为 -1。 */
  [[nodiscard]] int cellAt(Point point) const;

  Label* leading_{nullptr};
  Label* title_{nullptr};
  std::vector<Label*> cells_;
  std::vector<RowCell> cellSpecs_;
  /** 各列的点击回调（与是否可用），与 cells_ 一一对应；没有回调的列不可点击。 */
  std::vector<std::function<void(const Rect&)>> cellHandlers_;
  std::vector<bool> cellEnabled_;
  /** 鼠标悬停的、鼠标按下的可点击列，没有为 -1。 */
  int hoverCell_{-1};
  int pressedCell_{-1};
  Widget* actions_{nullptr};
  bool hasLeading_{false};
  std::optional<IconId> leadingIcon_;
  double leadingWidth_{0};
  bool leadingEmphasized_{false};
  /** 前导图标的位置，每次摆放时更新。 */
  Rect leadingIconRect_;
};

}  // namespace rujian::ui
