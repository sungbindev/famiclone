/*
 * ------------------------------------------------------------------------
 * 名称：tree.hpp
 * 说明：树：一列可选择的条目，每项占一行，鼠标点击或键盘选择；整棵树只有一个 Tab 停靠点，方向键在条目间移动焦点。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Tree，只做 Ember 用到的部分（扁平的条目、选中条目与选中回调）；子节点展开、数量、拖放与右键菜单没用到，不做，
 *       但每行仍留出展开箭头的位置，名称与组件库对齐。条目放在滚动区域里时，焦点移动会让滚动区域把焦点行滚进可见范围。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 树的一个条目。 */
struct TreeItem {
  /** 唯一标识。 */
  std::string id;
  /** 名称。 */
  std::string label;
};

/** 树。 */
class Tree : public Widget {
 public:
  /** 选中条目时调用，参数是条目的标识。 */
  using SelectHandler = std::function<void(const std::string&)>;

  /** 设置全部条目；选中项不在其中时视为没有选中。 */
  void setItems(std::vector<TreeItem> items);
  /** 选中指定条目，空串表示没有选中（不调用回调）。 */
  void setSelectedId(std::string id);
  [[nodiscard]] const std::string& selectedId() const { return selectedId_; }
  void setOnSelect(SelectHandler handler);

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
  [[nodiscard]] Rect rowRect(std::size_t index) const;
  [[nodiscard]] int indexAt(Point point) const;
  /** 可获得焦点的行：移动过焦点的取它，否则取选中项，再否则取第一项。 */
  [[nodiscard]] std::size_t focusIndex() const;
  void moveFocus(std::size_t index);
  void choose(std::size_t index);

  std::vector<TreeItem> items_;
  std::string selectedId_;
  SelectHandler onSelect_;
  int focusRow_{-1};
  int hoverRow_{-1};
  int pressedRow_{-1};
};

}  // namespace rujian::ui
