/*
 * ------------------------------------------------------------------------
 * 名称：table.hpp
 * 说明：数据表格（紧凑、平铺外观）：表头加若干行，列宽按内容自动分配；行可以选中，选中的行下方可以插入一整行的详情。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Table，只做 Ember 用到的部分：紧凑与平铺（嵌在面板或对话框里，没有卡片边框与圆角）两个属性固定为开，
 *       文字列（最小宽度、不换行、弱化色）、行选中、行详情；数字列、操作列、列预设、右键菜单、悬停提示（tooltip）没用到，不做。
 *       平铺表格的表头在组件库里粘在滚动区域顶部，这里暂时不粘（见 08 第 10 节）。
 *       列宽的分配规则：放得下时多出的宽度只分给设了伸展（grow）的列，都没设时按最大内容宽度的比例分给全部列；放不下时在最小与最大宽度之间按比例，再不够就取最小宽度。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class Label;

/** 表格的一列。 */
struct TableColumn {
  /** 表头文字。 */
  std::string title;
  /** 列的最小宽度（逻辑像素），0 表示没有。 */
  double minWidth{0};
  /** 内容不换行。 */
  bool nowrap{false};
  /** 内容用弱化色。 */
  bool muted{false};
  /** 列的最大宽度（逻辑像素），0 表示不限；超出的内容换行。 */
  double maxWidth{0};
  /** 多出的宽度只分给设了伸展的列；所有列都没设时按最大宽度比例分给全部列。 */
  bool grow{false};
};

/** 表格。 */
class Table : public Widget {
 public:
  /** 点击某一行时调用，参数是行号（从 0 起）。 */
  using SelectHandler = std::function<void(std::size_t)>;

  Table();

  /** 加一列，必须在加行之前。 */
  void addColumn(TableColumn column);
  /**
   * 加一行，单元格个数必须与列数相同；返回行号。
   * 文字单元格用 makeText 创建，列的不换行与弱化色设置会应用到它们上。
   */
  std::size_t addRow(std::vector<std::unique_ptr<Widget>> cells);
  /** 给指定行加详情：显示在该行正下方，跨所有列。 */
  void setRowDetail(std::size_t row, std::unique_ptr<Widget> detail);
  /** 创建一个文字单元格（字号、行高取表格令牌）。 */
  [[nodiscard]] static std::unique_ptr<Label> makeText(std::string text);

  /** 当前选中的行，-1 表示没有；设置了选中回调后行才可选。 */
  void setSelectedRow(int row);
  void setOnRowSelect(SelectHandler handler);

  [[nodiscard]] Size measure(Size available) override;
  /** 各列都取最小宽度时的表格宽度：再窄列就放不下了（文字超出用省略号）。 */
  [[nodiscard]] Size minimumSize() override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] CursorKind cursor() const override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  void onStateChanged() override;

 private:
  /** 按可用宽度算出的布局：列宽与各行的纵向位置。 */
  struct Layout {
    std::vector<double> columnWidths;
    double headerHeight{0};
    std::vector<double> rowTops;
    std::vector<double> rowHeights;
    /** 每行详情的顶边与高度（没有详情时高度为 0）。 */
    std::vector<double> detailTops;
    std::vector<double> detailHeights;
    double height{0};
  };

  /** 一行：单元格与详情。 */
  struct Row {
    std::vector<Widget*> cells;
    Widget* detail{nullptr};
  };

  [[nodiscard]] Layout layout(double width);
  /** 每列的最小宽度与最大（内容不换行时的）宽度。 */
  void columnLimits(std::vector<double>& minimum, std::vector<double>& maximum);
  [[nodiscard]] double borderWidth() const;
  [[nodiscard]] int rowAt(Point point);

  std::vector<TableColumn> columns_;
  std::vector<Label*> headers_;
  std::vector<Row> rows_;
  int selectedRow_{-1};
  int hoverRow_{-1};
  int pressedRow_{-1};
  SelectHandler onSelect_;
};

/** 主副文字单元格：加粗的主文字，下方是弱化色的说明；两者都可以换行，宽度随所在列。 */
class TableMainCell : public Widget {
 public:
  TableMainCell();

  void setText(std::string text);
  void setDescription(std::string description);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  Label* text_{nullptr};
  Label* description_{nullptr};
};

}  // namespace rujian::ui
