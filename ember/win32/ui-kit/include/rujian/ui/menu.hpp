/*
 * ------------------------------------------------------------------------
 * 名称：menu.hpp
 * 说明：菜单：菜单项的数据、菜单弹层（勾选、单选、图标、快捷键与提示文字、分隔线、子菜单、助记符、键盘导航）与菜单栏。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 MenuBar、MenuPopup 与 MenuItem。键盘焦点留在打开菜单的控件（菜单栏）上，按键由它转给弹层处理；
 *       弹层经 Host::showLayer 显示成独立小窗口（不受窗口边界裁剪），子菜单是各自的弹层，定位边界是 Host::layerViewport；
 *       弹层高度放不下时不滚动（Ember 的菜单项都不多），见 08 第 10 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rujian/ui/icon-id.hpp"
#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 菜单项。 */
struct MenuItem {
  /** 标识，回调时传回。 */
  std::string id;
  /** 显示文字。 */
  std::string label;
  /** 勾选状态：没有值表示普通项，有值表示可勾选项（radio 为真时是单选项）。 */
  std::optional<bool> checked;
  /** 单选项：选中后关闭菜单，勾选状态由使用方更新。 */
  bool radio{false};
  bool disabled{false};
  /** 文字前的图标。 */
  std::optional<IconId> icon;
  /** 右侧显示的快捷键文字，如 Ctrl+B。 */
  std::string shortcut;
  /** 在这一项前画分隔线。 */
  bool separatorBefore{false};
  /** 助记字母：显示在文字后的括号里，菜单打开时按它直接选中。 */
  std::string mnemonic;
  /** 右侧显示的提示文字（如“暂未支持”），位置同快捷键；有快捷键时不显示。 */
  std::string hint;
  /** 子菜单项：非空时这一项是子菜单，选中时在右侧展开，本身不再触发选中。 */
  std::vector<MenuItem> items;
};

/** 菜单项显示的文字：有助记字母时写成“名称(X)”。 */
[[nodiscard]] std::string menuItemText(const MenuItem& item);

/** 菜单弹层的使用方：接收选中、关闭、切换菜单与点到弹层之外。 */
class MenuPopupOwner {
 public:
  virtual ~MenuPopupOwner() = default;
  /** 选中了一个叶子项，path 是从顶层开始逐级的索引。 */
  virtual void menuActivated(const std::vector<std::size_t>& path) = 0;
  /** 请求关闭整个菜单；returnFocus 为真时应把焦点还给打开菜单的控件。 */
  virtual void menuClosed(bool returnFocus) = 0;
  /** 顶层弹层里按了左右方向键：切到相邻的菜单（delta 为 -1 或 1）；不支持切换时什么也不做。 */
  virtual void menuSwitch(int delta) = 0;
  /** 在弹层之外按下了鼠标（hit 为被按到的控件）。 */
  virtual void menuOutsidePress(Widget* hit) = 0;
};

/** 菜单弹层（也可以是子菜单）。 */
class MenuPopup final : public Widget {
 public:
  MenuPopup(MenuPopupOwner& owner, std::vector<MenuItem> items, std::vector<std::size_t> path = {}, MenuPopup* parent = nullptr);
  ~MenuPopup() override;

  /** 最小宽度（含边框）。 */
  void setMinWidth(double minWidth);
  /** 作为顶层菜单显示在触发器下方左对齐，弹层可用范围内放不下时翻到上方；必须先 showLayer。 */
  void placeBelow(const Rect& anchor);
  /** 作为子菜单显示在父项右侧（弹层可用范围内放不下改到左侧，两侧都放不下时盖住父菜单的一部分并缩窄）；必须先 showLayer。 */
  void placeBeside(const Rect& parentItem);

  /** 键盘高亮的项，-1 表示没有。 */
  void setActive(int index);
  [[nodiscard]] int active() const { return active_; }
  /** 处理一个按键，返回是否已处理；由菜单的拥有者在自己收到按键时调用。 */
  bool handleKey(const KeyEvent& event);
  /** 处理一个字符（助记字母），返回是否已处理。 */
  bool handleChar(char32_t codePoint);
  /** 关掉子菜单。 */
  void closeSub();
  /** 控件是否在本弹层或它展开的子菜单里。 */
  [[nodiscard]] bool containsTree(const Widget* widget) const;
  /** 第 index 项的区域（窗口坐标）。 */
  [[nodiscard]] Rect itemRect(std::size_t index) const;
  [[nodiscard]] std::size_t itemCount() const { return items_.size(); }
  /** 展开的子菜单（没有时为空）。 */
  [[nodiscard]] MenuPopup* sub() const { return sub_.get(); }

  [[nodiscard]] Size measure(Size available) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] CursorKind cursor() const override { return CursorKind::Arrow; }
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  void onStateChanged() override;

 private:
  /** 一项的位置与高度（相对弹层内容区顶边）。 */
  struct Row {
    double top{0};
    double height{0};
  };

  [[nodiscard]] double border() const;
  [[nodiscard]] double itemWidth(const MenuItem& item);
  [[nodiscard]] double naturalWidth();
  [[nodiscard]] double naturalHeight() const;
  [[nodiscard]] std::vector<Row> rows() const;
  [[nodiscard]] int indexAt(Point point) const;
  [[nodiscard]] MenuPopup* keyboardTarget();
  void activate(std::size_t index);
  void openSub(std::size_t index, bool focusFirst);
  void move(int delta);
  bool keyHere(const KeyEvent& event);
  bool charHere(char32_t codePoint);

  MenuPopupOwner& owner_;
  MenuPopup* parent_;
  std::vector<MenuItem> items_;
  std::vector<std::size_t> path_;
  double minWidth_{0};
  int active_{-1};
  int subIndex_{-1};
  int downIndex_{-1};
  std::unique_ptr<MenuPopup> sub_;
};

/** 菜单栏中的一个菜单。 */
struct MenuBarMenu {
  std::string id;
  std::string label;
  std::string mnemonic;
  std::vector<MenuItem> items;
};

/** 菜单栏。 */
class MenuBar : public Widget, public MenuPopupOwner {
 public:
  /** 选中普通项或单选项时以 (菜单标识, 菜单项标识) 调用；切换可勾选项时再带切换后的勾选状态。 */
  using SelectHandler = std::function<void(const std::string& menuId, const std::string& itemId, std::optional<bool> checked)>;
  /** 打开的菜单变化时调用，空串表示都没打开。 */
  using OpenHandler = std::function<void(const std::string& menuId)>;

  MenuBar();
  ~MenuBar() override;

  void setMenus(std::vector<MenuBarMenu> menus);
  void setOnSelect(SelectHandler handler);
  void setOnOpenChange(OpenHandler handler);
  /** 打开指定菜单（itemIndex 是高亮项，-1 表示不高亮）；菜单不存在或没有项时不变。 */
  void openMenu(const std::string& menuId, int itemIndex = 0);
  void closeMenu(bool returnFocus = false);
  [[nodiscard]] const std::string& openMenuId() const { return openId_; }
  /** 打开的菜单弹层（没打开时为空）。 */
  [[nodiscard]] MenuPopup* popup() const { return popup_.get(); }
  /** 第 index 个菜单按钮的区域（窗口坐标）。 */
  [[nodiscard]] Rect buttonRect(std::size_t index) const;

  [[nodiscard]] Size measure(Size available) override;
  void paint(Canvas& canvas) override;
  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return CursorKind::Arrow; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }
  bool onMouseDown(const MouseEvent& event) override;
  bool onMouseUp(const MouseEvent& event) override;
  bool onMouseMove(const MouseEvent& event) override;
  bool onKeyDown(const KeyEvent& event) override;
  bool onChar(char32_t codePoint) override;
  void onStateChanged() override;

  void menuActivated(const std::vector<std::size_t>& path) override;
  void menuClosed(bool returnFocus) override;
  void menuSwitch(int delta) override;
  void menuOutsidePress(Widget* hit) override;

 private:
  [[nodiscard]] double buttonWidth(std::size_t index) const;
  [[nodiscard]] int indexAt(Point point) const;
  void openAt(std::size_t index, int itemIndex);

  std::vector<MenuBarMenu> menus_;
  std::string openId_;
  std::unique_ptr<MenuPopup> popup_;
  SelectHandler onSelect_;
  OpenHandler onOpenChange_;
  int focusIndex_{0};
  int hoverIndex_{-1};
  int pressedIndex_{-1};
};

}  // namespace rujian::ui
