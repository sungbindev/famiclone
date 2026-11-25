/*
 * ------------------------------------------------------------------------
 * 名称：tree.hpp
 * 说明：树：一列可选择的条目，每项占一行，鼠标点击或键盘选择；可以按层级展开与收起，条目可带图标与靠右的说明文字；整棵树只有一个 Tab 停靠点，方向键在条目间移动焦点。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Tree，只做 Ember 用到的部分（条目、层级、展开与收起、选中条目与选中回调、图标、行尾文字、右键菜单回调）；数量、拖放没用到，不做。
 *       条目按“先序”排成一列并给出层级（depth）：下一项比这一项深，这一项就有子项，子项收起时不显示；每行左边留出展开箭头的位置（没有子项时空着），名称与组件库对齐。
 *       条目放在滚动区域里时，焦点移动会让滚动区域把焦点行滚进可见范围。
 *       键盘：上下移动焦点；右键展开，已展开时移到第一个子项；左键收起，已收起或没有子项时移到父项；回车与空格选中；Shift+F10 与菜单键触发右键菜单回调。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/icon-id.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 树的一个条目。 */
struct TreeItem {
  /** 唯一标识。 */
  std::string id;
  /** 名称。 */
  std::string label;
  /** 层级，最外层是 0；条目按先序排列，下一项的层级更深时这一项有子项。 */
  int depth{0};
  /** 名称前的图标；没有时不画。 */
  std::optional<IconId> icon;
  /** 靠右显示的说明文字（弱化色）；没有时为空。 */
  std::string trailing;
};

/** 树。 */
class Tree : public Widget {
 public:
  /** 选中条目时调用，参数是条目的标识。 */
  using SelectHandler = std::function<void(const std::string&)>;
  /** 右键点击或按右键菜单键时调用：id 是条目的标识（点在空白处时为空串），position 是菜单要弹出的位置（窗口坐标）。 */
  using ContextHandler = std::function<void(const std::string& id, Point position)>;

  /** 设置全部条目（先序）；选中项不在其中时视为没有选中；展开状态按条目标识保留。 */
  void setItems(std::vector<TreeItem> items);
  /** 选中指定条目，空串表示没有选中（不调用回调）。 */
  void setSelectedId(std::string id);
  [[nodiscard]] const std::string& selectedId() const { return selectedId_; }
  void setOnSelect(SelectHandler handler);
  void setOnContextMenu(ContextHandler handler);

  /** 没有明确展开或收起过的条目是否展开，默认收起。 */
  void setDefaultExpanded(bool expanded);
  /** 展开或收起一个条目。 */
  void setExpanded(const std::string& id, bool expanded);
  [[nodiscard]] bool isExpanded(const std::string& id) const;
  /** 此刻显示的行数（收起的子项不算）。 */
  [[nodiscard]] std::size_t visibleCount() const { return rows_.size(); }
  /** 第 index 个显示的行的条目标识。 */
  [[nodiscard]] const std::string& visibleId(std::size_t index) const { return items_[rows_[index]].id; }

  [[nodiscard]] Size measure(Size available) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return CursorKind::Hand; }
  [[nodiscard]] Rect focusRect() const override;
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  void onStateChanged() override;

 private:
  [[nodiscard]] Rect rowRect(std::size_t visibleIndex) const;
  [[nodiscard]] int indexAt(Point point) const;
  /** 第 visibleIndex 行的展开箭头的区域。 */
  [[nodiscard]] Rect arrowRect(std::size_t visibleIndex) const;
  [[nodiscard]] bool hasChildren(std::size_t itemIndex) const;
  /** 可获得焦点的行：移动过焦点的取它，否则取选中项，再否则取第一项。 */
  [[nodiscard]] std::size_t focusIndex() const;
  /** 按展开状态重新算出显示的行。 */
  void rebuildRows();
  void moveFocus(std::size_t visibleIndex);
  void choose(std::size_t visibleIndex);
  void toggle(std::size_t visibleIndex);
  void showContext(const std::string& id, Point position);

  std::vector<TreeItem> items_;
  /** 显示的行：items_ 的下标。 */
  std::vector<std::size_t> rows_;
  std::map<std::string, bool> expanded_;
  bool defaultExpanded_{false};
  std::string selectedId_;
  std::string focusId_;
  SelectHandler onSelect_;
  ContextHandler onContext_;
  int hoverRow_{-1};
  int pressedRow_{-1};
  int rightPressedRow_{-1};
};

}  // namespace rujian::ui
