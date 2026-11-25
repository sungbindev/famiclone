/*
 * ------------------------------------------------------------------------
 * 名称：tree.hpp
 * 说明：树：一列可选择的条目，每项占一行，鼠标点击或键盘选择；可以按层级展开与收起，条目可带图标与靠右的说明文字；整棵树只有一个 Tab 停靠点，方向键在条目间移动焦点。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Tree，只做 Ember 用到的部分（条目、层级、展开与收起、选中条目与选中回调、图标、行尾文字、右键菜单回调、行内重命名、按键回调、拖动移动）；数量没用到，不做。
 *       条目按“先序”排成一列并给出层级（depth）：下一项比这一项深，这一项就有子项，子项收起时不显示；每行左边留出展开箭头的位置（没有子项时空着），名称与组件库对齐。
 *       条目放在滚动区域里时，焦点移动会让滚动区域把焦点行滚进可见范围；光标只在鼠标悬停在某一行上时是手形，空白处不变。
 *       键盘：上下移动焦点；右键展开，已展开时移到第一个子项；左键收起，已收起或没有子项时移到父项；回车与空格选中；Shift+F10 与菜单键触发右键菜单回调；其余按键先交给“按键处理”回调（重命名、删除、剪切复制粘贴由使用方决定）。
 *       点击展开：打开 setToggleOnClick 后，单击有子项的行既选中它也展开或收起，不再用双击。
 *       拖动：设置“放下”回调后，在某一行上按下并拖过一小段距离开始拖动；放在文件夹行上就是放进这个文件夹，放在非文件夹行上是放进它的父文件夹，放在空白处是放进最外层（目标标识为空串）；“能否放下”回调返回假时不标出目标，放下也不调用。
 *       行内重命名：beginRename 在名称上放一个输入框，回车或失去焦点提交，Esc 取消。
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

class TextInput;

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
  /** 是文件夹（空的也算）：拖动时可以放进它里面。 */
  bool folder{false};
};

/** 树。 */
class Tree : public Widget {
 public:
  /** 选中条目时调用，参数是条目的标识。 */
  using SelectHandler = std::function<void(const std::string&)>;
  /** 右键点击或按右键菜单键时调用：id 是条目的标识（点在空白处时为空串），position 是菜单要弹出的位置（窗口坐标）。 */
  using ContextHandler = std::function<void(const std::string& id, Point position)>;
  /** 行内重命名提交：名称没有改变时不调用。 */
  using RenameHandler = std::function<void(const std::string& id, const std::string& name)>;
  /** 按键处理：返回 true 表示已处理。 */
  using KeyHandler = std::function<bool(const KeyEvent& event)>;
  /** 拖动放下：id 是被拖的条目，targetFolder 是放进的文件夹（空串是最外层）。 */
  using DropHandler = std::function<void(const std::string& id, const std::string& targetFolder)>;
  /** 拖动经过时询问能不能放进这个文件夹。 */
  using CanDropHandler = std::function<bool(const std::string& id, const std::string& targetFolder)>;

  Tree();

  /** 设置全部条目（先序）；选中项不在其中时视为没有选中；展开状态按条目标识保留；正在重命名的条目没了就取消重命名。 */
  void setItems(std::vector<TreeItem> items);
  /** 选中指定条目，空串表示没有选中（不调用回调）。 */
  void setSelectedId(std::string id);
  [[nodiscard]] const std::string& selectedId() const { return selectedId_; }
  /** 有键盘焦点的条目：移动过焦点的取它，否则取选中项，再否则取第一项；没有条目时为空串。 */
  [[nodiscard]] std::string focusedId() const;
  void setOnSelect(SelectHandler handler);
  void setOnContextMenu(ContextHandler handler);
  void setOnRename(RenameHandler handler) { onRename_ = std::move(handler); }
  void setOnKey(KeyHandler handler) { onKey_ = std::move(handler); }
  void setOnDrop(DropHandler handler) { onDrop_ = std::move(handler); }
  void setCanDrop(CanDropHandler handler) { canDrop_ = std::move(handler); }
  /** 单击有子项的行时同时展开或收起它（默认只有点箭头或双击才展开或收起）。 */
  void setToggleOnClick(bool enabled) { toggleOnClick_ = enabled; }

  /** 开始重命名一个条目（条目没有显示时先展开它的祖先；不存在时什么都不做）。 */
  void beginRename(const std::string& id);
  void cancelRename();
  [[nodiscard]] bool renaming() const { return !renameId_.empty(); }
  /** 展开一个条目的全部祖先，让它显示出来。 */
  void reveal(const std::string& id);

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
  void arrange(const Rect& bounds) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return hoverRow_ >= 0 ? CursorKind::Hand : CursorKind::Arrow; }
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
  /** 重命名输入框的位置：盖在正在重命名的行的名称上。 */
  void layoutEditor();
  void commitRename();
  /** 正在重命名的条目在 items_ 里的下标。 */
  [[nodiscard]] int itemIndexOf(const std::string& id) const;
  /** 拖动经过 point 时对应的目标文件夹（空串是最外层）。 */
  [[nodiscard]] std::string dropTargetAt(Point point) const;
  void updateDrag(Point point);
  void finishDrag(Point point);

  std::vector<TreeItem> items_;
  /** 显示的行：items_ 的下标。 */
  std::vector<std::size_t> rows_;
  std::map<std::string, bool> expanded_;
  bool defaultExpanded_{false};
  std::string selectedId_;
  std::string focusId_;
  SelectHandler onSelect_;
  ContextHandler onContext_;
  RenameHandler onRename_;
  KeyHandler onKey_;
  DropHandler onDrop_;
  CanDropHandler canDrop_;
  bool toggleOnClick_{false};
  int hoverRow_{-1};
  int pressedRow_{-1};
  int rightPressedRow_{-1};
  /** 拖动：按下的位置、是否已开始、被拖的条目、此刻标出的目标文件夹与它能否放下。 */
  Point pressPosition_{};
  bool dragging_{false};
  std::string dragId_;
  std::string dropFolder_;
  bool dropValid_{false};

  TextInput* editor_{nullptr};
  std::string renameId_;
  std::string renameOriginal_;
};

}  // namespace rujian::ui
