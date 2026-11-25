/*
 * ------------------------------------------------------------------------
 * 名称：file-table.hpp
 * 说明：文件列表：表头加若干行（图标与名称在第一列，其余列是文字），可多选，支持键盘导航、行内重命名、右键菜单回调、拖出与拖入时的行高亮；没有边框线，行紧凑。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：Ember 自己的控件，网页版没有。行的内容与顺序由使用方给出（每行有唯一标识）；放在滚动区域里使用：表头固定在滚动区域的顶部，
 *       内容不够高时控件撑满整个可见区域（空白处也能点击与右键）。
 *       选中规则同系统文件管理器：点击选中一行，Ctrl+点击增减，Shift+点击选中范围，点在已选中的行上（没有拖动）时才收成只选这一行；
 *       右键点在没选中的行上先选中它，点在空白处清空选择；没有全选。
 *       键盘：上下、Home、End、翻页键移动焦点并选中（带 Shift 扩展选区，带 Ctrl 只移动焦点），Ctrl+空格增减选中，回车触发“打开”回调，
 *       Shift+F10 与菜单键触发右键菜单回调；其余按键先交给“按键处理”回调（重命名、删除、剪切复制粘贴、撤销等由使用方决定）。
 *       拖动：在已选中的行上按下并拖过一小段距离，用选中的行标识调用“开始拖动”回调（真正的拖放由应用完成）。
 *       行内重命名：beginRename 在名称单元格上放一个输入框，回车或失去焦点提交，Esc 取消。
 *       列宽：除最后一列外每列都有自己的宽度（名称列也是），最后一列占满剩余宽度；表头里相邻两列之间有竖线，鼠标在线上变成左右调整光标并停留 200 毫秒后变成聚焦色（与分栏的分隔条一致），拖动它改变线左边那一列的宽度，线右边的列跟着移动，最后一列吸收差值；调过的列宽按表头文字记住，重新 setColumns 后仍然有效。
 *       靠右对齐的列（如大小）在它不是最后一列时表头与内容都靠右，是最后一列时靠左，这样竖线右边总是紧挨着文字。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "rujian/ui/icon-id.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

class TextInput;

/** 文件列表的一列。除最后一列外每列都按给定宽度排（名称列是第一列，图标加名称），最后一列占满剩余宽度。 */
struct FileTableColumn {
  /** 表头文字。 */
  std::string title;
  /** 列宽（逻辑像素）；最后一列忽略它，名称列为 0 时用默认宽度。 */
  double width{0};
  /** 内容靠右对齐（如大小）；最后一列不受它影响，一律靠左。 */
  bool alignEnd{false};
};

/** 文件列表的一行。 */
struct FileTableRow {
  /** 唯一标识。 */
  std::string id;
  /** 名称前的图标；没有时不画。 */
  std::optional<IconId> icon;
  /** 各列的文字，第一个是名称；个数可以少于列数。 */
  std::vector<std::string> cells;
};

/** 文件列表。 */
class FileTable : public Widget {
 public:
  /** 参数是一行的标识。 */
  using RowHandler = std::function<void(const std::string& id)>;
  /** 右键点击或按菜单键：ids 是此刻选中的行（点在空白处时为空），position 是菜单要弹出的位置（窗口坐标）。 */
  using ContextHandler = std::function<void(const std::vector<std::string>& ids, Point position)>;
  /** 开始拖动：ids 是选中的行（按行的顺序）。 */
  using DragHandler = std::function<void(const std::vector<std::string>& ids)>;
  /** 行内重命名提交：名称没有改变时不调用。 */
  using RenameHandler = std::function<void(const std::string& id, const std::string& name)>;
  /** 按键处理：返回 true 表示已处理。 */
  using KeyHandler = std::function<bool(const KeyEvent& event)>;

  FileTable();

  /** 设置各列；必须在设置行之前。 */
  void setColumns(std::vector<FileTableColumn> columns);
  /** 设置全部行；仍存在的行保持选中与焦点，不存在的从选中里去掉；正在重命名的行没了就取消重命名。 */
  void setRows(std::vector<FileTableRow> rows);
  /** 没有行时在中间显示的说明文字。 */
  void setEmptyText(std::string text);

  /** 此刻选中的行标识，按行的顺序。 */
  [[nodiscard]] std::vector<std::string> selectedIds() const;
  /** 设置选中的行（不调用选中变化回调）。 */
  void setSelection(const std::vector<std::string>& ids);
  /** 有键盘焦点的行；没有行时为空串。 */
  [[nodiscard]] std::string focusId() const;

  void setOnActivate(RowHandler handler) { onActivate_ = std::move(handler); }
  void setOnSelectionChange(std::function<void()> handler) { onSelectionChange_ = std::move(handler); }
  void setOnContextMenu(ContextHandler handler) { onContext_ = std::move(handler); }
  void setOnDragStart(DragHandler handler) { onDragStart_ = std::move(handler); }
  void setOnRename(RenameHandler handler) { onRename_ = std::move(handler); }
  void setOnKey(KeyHandler handler) { onKey_ = std::move(handler); }

  /** 开始重命名一行（行不存在时什么都不做）。 */
  void beginRename(const std::string& id);
  void cancelRename();
  [[nodiscard]] bool renaming() const { return !renameId_.empty(); }

  /** 窗口坐标 point 所在的行的标识；表头、空白处没有。 */
  [[nodiscard]] std::optional<std::string> rowIdAt(Point point) const;
  /** 一行的区域（窗口坐标）；行不存在时为空区域。 */
  [[nodiscard]] Rect rowRect(const std::string& id) const;
  /** 标出拖放时会落进去的行；空串表示没有。 */
  void setDropHighlight(const std::string& id);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return hoverDivider_ >= 0 || resizeIndex_ >= 0 ? CursorKind::SizeWE : CursorKind::Arrow; }
  [[nodiscard]] Rect focusRect() const override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;
  void onTimer(unsigned id) override;

 private:
  /** 各列的宽度（最后一列占满剩余）。 */
  [[nodiscard]] std::vector<double> columnWidths() const;
  /** 可见区域：放在滚动区域里时是滚动区域的区域，否则是自己的区域。 */
  [[nodiscard]] Rect viewport() const;
  /** 表头里第 index 条列分割线（index 和 index+1 两列之间）的 x（窗口坐标）。 */
  [[nodiscard]] double dividerX(std::size_t index, const std::vector<double>& widths) const;
  /** point 在哪条列分割线上；不在表头里或不在线上为 -1。 */
  [[nodiscard]] int dividerAt(Point point) const;
  void resizeTo(double x);
  /** 改变鼠标停着的列分割线；停留一会儿后才显示聚焦色。 */
  void setHoverDivider(int divider);
  [[nodiscard]] Rect headerRect() const;
  [[nodiscard]] Rect rowRectAt(std::size_t index) const;
  [[nodiscard]] int indexAt(Point point) const;
  [[nodiscard]] int indexOf(const std::string& id) const;
  [[nodiscard]] std::size_t focusIndex() const;
  [[nodiscard]] bool selected(std::size_t index) const { return selection_.count(rows_[index].id) != 0; }
  [[nodiscard]] Rect nameCellRect(std::size_t index) const;

  /** 只选中一行。 */
  void selectOnly(std::size_t index);
  /** 选中从锚点到 index 的范围（替换原来的选区）。 */
  void selectRange(std::size_t index);
  void moveFocus(std::size_t index);
  /** 键盘移动：extend 带 Shift 扩展选区，keepSelection 带 Ctrl 只移动焦点。 */
  void navigate(std::size_t index, bool extend, bool keepSelection);
  void notifySelection();
  void showContext(Point position);
  void commitRename();
  void layoutEditor();

  std::vector<FileTableColumn> columns_;
  /** 用户拖过的列宽，按表头文字记住。 */
  std::map<std::string, double> userWidths_;
  int hoverDivider_{-1};
  /** 鼠标停着的分割线是否已经停留够久、显示聚焦色。 */
  bool dividerShown_{false};
  unsigned dividerTimer_{0};
  /** 正在拖的列分割线；没有为 -1。 */
  int resizeIndex_{-1};
  double resizeStartX_{0};
  std::vector<double> resizeStartWidths_;
  std::vector<FileTableRow> rows_;
  std::string emptyText_;
  std::set<std::string> selection_;
  std::string anchorId_;
  std::string focusId_;
  std::string dropId_;
  int hoverRow_{-1};
  /** 左键按下的行与位置；松开时没有拖动才把选区收成这一行（点在已选中的行上时）。 */
  int pressedRow_{-1};
  Point pressPosition_{};
  bool collapseOnRelease_{false};
  bool dragged_{false};
  bool suppressRelease_{false};
  bool rightPressed_{false};
  RowHandler onActivate_;
  std::function<void()> onSelectionChange_;
  ContextHandler onContext_;
  DragHandler onDragStart_;
  RenameHandler onRename_;
  KeyHandler onKey_;

  TextInput* editor_{nullptr};
  std::string renameId_;
  std::string renameOriginal_;
};

}  // namespace rujian::ui
