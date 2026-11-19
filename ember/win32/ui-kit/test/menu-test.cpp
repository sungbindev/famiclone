/*
 * ------------------------------------------------------------------------
 * 名称：menu-test.cpp
 * 说明：菜单栏与菜单弹层的测试：点击与悬停打开和切换、弹层的位置与排版、鼠标选中、禁用项、可勾选项保持打开、子菜单、键盘导航、助记符、点击菜单外关闭。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：弹层里菜单项的位置用 MenuPopup::itemRect 取得。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "rujian/ui/menu.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

constexpr int kVkTab = 0x09;
constexpr int kVkReturn = 0x0D;
constexpr int kVkEscape = 0x1B;
constexpr int kVkEnd = 0x23;
constexpr int kVkHome = 0x24;
constexpr int kVkLeft = 0x25;
constexpr int kVkUp = 0x26;
constexpr int kVkRight = 0x27;
constexpr int kVkDown = 0x28;

MenuItem item(std::string id, std::string label) { return MenuItem{.id = std::move(id), .label = std::move(label)}; }

std::vector<MenuBarMenu> sampleMenus(bool fullscreen = false) {
  MenuItem exit = item("exit", "退出");
  exit.separatorBefore = true;
  exit.mnemonic = "x";
  MenuItem open = item("open", "打开");
  open.disabled = true;
  MenuItem create = item("new", "新建");
  create.shortcut = "Ctrl+N";
  MenuItem full = item("full", "全屏");
  full.checked = fullscreen;
  MenuItem zoom = item("zoom", "缩放");
  zoom.items = {item("zoom-in", "放大"), item("zoom-out", "缩小")};
  MenuItem theme = item("dark", "深色");
  theme.checked = true;
  theme.radio = true;
  return {
      MenuBarMenu{.id = "file", .label = "文件", .mnemonic = "F", .items = {create, open, exit}},
      MenuBarMenu{.id = "view", .label = "视图", .mnemonic = "V", .items = {full, zoom, theme}},
  };
}

struct Selection {
  std::string menu;
  std::string item;
  std::optional<bool> checked;
};

/** 把菜单栏放在 (10, 10)，页面 600x300。 */
class Holder final : public Widget {
 public:
  void arrange(const Rect& bounds) override {
    Widget::arrange(bounds);
    for (const auto& child : children()) {
      const Size size = child->measure(Size{});
      child->arrange(Rect{10, 10, size.width, size.height});
    }
  }
};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  MenuBar* bar{nullptr};
  std::vector<Selection> selections;
  std::vector<std::string> opened;

  Scene() {
    auto holder = std::make_unique<Holder>();
    auto menuBar = std::make_unique<MenuBar>();
    bar = menuBar.get();
    holder->addChild(std::move(menuBar));
    root.setContent(std::move(holder));
    bar->setMenus(sampleMenus());
    bar->setOnSelect([this](const std::string& menu, const std::string& id, std::optional<bool> checked) { selections.push_back({menu, id, checked}); });
    bar->setOnOpenChange([this](const std::string& id) { opened.push_back(id); });
    root.layout(Size{600, 300});
  }

  void click(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }

  static Point center(const Rect& rect) { return Point{rect.x + rect.width / 2, rect.y + rect.height / 2}; }

  [[nodiscard]] Point buttonCenter(std::size_t index) const { return center(bar->buttonRect(index)); }
  [[nodiscard]] Point itemCenter(std::size_t index) const { return center(bar->popup()->itemRect(index)); }
  [[nodiscard]] Point subItemCenter(std::size_t index) const { return center(bar->popup()->sub()->itemRect(index)); }

  void key(int virtualKey) { root.keyDown(virtualKey, 0, false); }
};

}  // namespace

TEST(Menu, ClickOpensTheMenuWithoutHighlightingAnyItem) {
  Scene s;
  s.click(s.buttonCenter(0));
  ASSERT_NE(s.bar->popup(), nullptr);
  EXPECT_EQ(s.bar->openMenuId(), "file");
  EXPECT_EQ(s.bar->popup()->active(), -1);
  EXPECT_EQ(s.opened, (std::vector<std::string>{"file"}));
  EXPECT_TRUE(s.bar->focused());
}

TEST(Menu, PopupSitsBelowTheButtonAndIsAtLeastAsWideAsIt) {
  Scene s;
  s.click(s.buttonCenter(0));
  const Rect button = s.bar->buttonRect(0);
  const Rect box = s.bar->popup()->bounds();
  EXPECT_DOUBLE_EQ(box.x, button.x);
  EXPECT_DOUBLE_EQ(box.y, button.bottom() + 2);
  EXPECT_GE(box.width, button.width);
}

TEST(Menu, ItemsAre24HighAndSeparatorsAdd11) {
  Scene s;
  s.click(s.buttonCenter(0));
  const MenuPopup* popup = s.bar->popup();
  EXPECT_DOUBLE_EQ(popup->itemRect(0).height, 24.0);
  EXPECT_DOUBLE_EQ(popup->itemRect(1).y - popup->itemRect(0).bottom(), 0.0);
  EXPECT_DOUBLE_EQ(popup->itemRect(2).y - popup->itemRect(1).bottom(), 11.0);
  EXPECT_DOUBLE_EQ(popup->itemRect(0).y, popup->bounds().y + 1 + 4);
}

TEST(Menu, HoveringAnotherMenuNameSwitchesWithoutHighlight) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.root.mouseMove(s.buttonCenter(1), 0);
  EXPECT_EQ(s.bar->openMenuId(), "view");
  EXPECT_EQ(s.bar->popup()->active(), -1);
}

TEST(Menu, ClickingTheOpenMenuNameClosesIt) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.click(s.buttonCenter(0));
  EXPECT_EQ(s.bar->popup(), nullptr);
  EXPECT_TRUE(s.bar->openMenuId().empty());
  EXPECT_EQ(s.opened.back(), "");
}

TEST(Menu, ClickingAnItemReportsItAndClosesTheMenu) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.click(s.itemCenter(0));
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].menu, "file");
  EXPECT_EQ(s.selections[0].item, "new");
  EXPECT_FALSE(s.selections[0].checked.has_value());
  EXPECT_EQ(s.bar->popup(), nullptr);
  EXPECT_TRUE(s.bar->focused());
}

TEST(Menu, DisabledItemsCannotBeSelected) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.click(s.itemCenter(1));
  EXPECT_TRUE(s.selections.empty());
  EXPECT_NE(s.bar->popup(), nullptr);
}

TEST(Menu, CheckableItemsToggleAndKeepTheMenuOpen) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.click(s.itemCenter(0));
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].item, "full");
  EXPECT_EQ(s.selections[0].checked, std::optional<bool>(true));
  EXPECT_NE(s.bar->popup(), nullptr);
}

TEST(Menu, RadioItemsCloseTheMenuWithoutACheckedValue) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.click(s.itemCenter(2));
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].item, "dark");
  EXPECT_FALSE(s.selections[0].checked.has_value());
  EXPECT_EQ(s.bar->popup(), nullptr);
}

TEST(Menu, ReplacingTheMenusWhileOpenKeepsItOpenWithTheNewContent) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.click(s.itemCenter(0));
  s.bar->setMenus(sampleMenus(true));
  ASSERT_NE(s.bar->popup(), nullptr);
  EXPECT_EQ(s.bar->openMenuId(), "view");
}

TEST(Menu, HoveringASubmenuItemOpensTheSubmenuBesideIt) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.root.mouseMove(s.itemCenter(1), 0);
  ASSERT_NE(s.bar->popup()->sub(), nullptr);
  const Rect parent = s.bar->popup()->itemRect(1);
  EXPECT_DOUBLE_EQ(s.bar->popup()->sub()->bounds().x, parent.right());
  EXPECT_EQ(s.bar->popup()->sub()->active(), -1);
  s.click(s.subItemCenter(1));
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].item, "zoom-out");
  EXPECT_EQ(s.bar->popup(), nullptr);
}

TEST(Menu, MovingToAPlainItemClosesTheSubmenu) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.root.mouseMove(s.itemCenter(1), 0);
  ASSERT_NE(s.bar->popup()->sub(), nullptr);
  s.root.mouseMove(s.itemCenter(0), 0);
  EXPECT_EQ(s.bar->popup()->sub(), nullptr);
}

TEST(Menu, ClickingASubmenuItemOpensTheSubmenuInsteadOfSelecting) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.click(s.itemCenter(1));
  EXPECT_TRUE(s.selections.empty());
  ASSERT_NE(s.bar->popup()->sub(), nullptr);
  EXPECT_EQ(s.bar->popup()->sub()->active(), 0);
}

TEST(Menu, ArrowKeysMoveAndWrap) {
  Scene s;
  s.click(s.buttonCenter(1));
  EXPECT_EQ(s.bar->popup()->active(), -1);
  s.key(kVkDown);
  EXPECT_EQ(s.bar->popup()->active(), 0);
  s.key(kVkDown);
  EXPECT_EQ(s.bar->popup()->active(), 1);
  s.key(kVkUp);
  s.key(kVkUp);
  EXPECT_EQ(s.bar->popup()->active(), 2);
  s.key(kVkDown);
  EXPECT_EQ(s.bar->popup()->active(), 0);
  s.key(kVkEnd);
  EXPECT_EQ(s.bar->popup()->active(), 2);
  s.key(kVkHome);
  EXPECT_EQ(s.bar->popup()->active(), 0);
}

TEST(Menu, EnterSelectsTheActiveItem) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.key(kVkReturn);
  EXPECT_TRUE(s.selections.empty());
  s.key(kVkDown);
  s.key(kVkReturn);
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].item, "new");
  EXPECT_EQ(s.bar->popup(), nullptr);
}

TEST(Menu, EscapeClosesAndReturnsTheFocusToTheBar) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.key(kVkEscape);
  EXPECT_EQ(s.bar->popup(), nullptr);
  EXPECT_TRUE(s.bar->focused());
}

TEST(Menu, RightOnASubmenuItemOpensItAndLeftOrEscapeClosesOnlyTheSubmenu) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.key(kVkDown);
  s.key(kVkDown);
  s.key(kVkRight);
  ASSERT_NE(s.bar->popup()->sub(), nullptr);
  EXPECT_EQ(s.bar->popup()->sub()->active(), 0);
  s.key(kVkDown);
  EXPECT_EQ(s.bar->popup()->sub()->active(), 1);
  s.key(kVkLeft);
  EXPECT_EQ(s.bar->popup()->sub(), nullptr);
  EXPECT_EQ(s.bar->popup()->active(), 1);
  s.key(kVkRight);
  ASSERT_NE(s.bar->popup()->sub(), nullptr);
  s.key(kVkEscape);
  EXPECT_EQ(s.bar->popup()->sub(), nullptr);
  EXPECT_NE(s.bar->popup(), nullptr);
}

TEST(Menu, EnterInASubmenuSelectsItsItem) {
  Scene s;
  s.click(s.buttonCenter(1));
  s.key(kVkDown);
  s.key(kVkDown);
  s.key(kVkRight);
  s.key(kVkReturn);
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].item, "zoom-in");
}

TEST(Menu, LeftAndRightInTheTopPopupSwitchMenus) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.key(kVkRight);
  EXPECT_EQ(s.bar->openMenuId(), "view");
  EXPECT_EQ(s.bar->popup()->active(), 0);
  s.key(kVkLeft);
  EXPECT_EQ(s.bar->openMenuId(), "file");
  s.key(kVkLeft);
  EXPECT_EQ(s.bar->openMenuId(), "view");
}

TEST(Menu, AMnemonicLetterSelectsTheMatchingItem) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.root.character(U'X');
  ASSERT_EQ(s.selections.size(), 1u);
  EXPECT_EQ(s.selections[0].item, "exit");
}

TEST(Menu, ClickingOutsideTheMenuClosesIt) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.click(Point{380, 280});
  EXPECT_EQ(s.bar->popup(), nullptr);
  EXPECT_TRUE(s.selections.empty());
}

TEST(Menu, TabClosesTheMenu) {
  Scene s;
  s.click(s.buttonCenter(0));
  s.key(kVkTab);
  EXPECT_EQ(s.bar->popup(), nullptr);
}

TEST(Menu, OpenMenuCanBeCalledFromOutsideAndFocusesTheBar) {
  Scene s;
  s.bar->openMenu("view");
  ASSERT_NE(s.bar->popup(), nullptr);
  EXPECT_EQ(s.bar->openMenuId(), "view");
  EXPECT_EQ(s.bar->popup()->active(), 0);
  EXPECT_TRUE(s.bar->focused());
  s.bar->closeMenu();
  EXPECT_EQ(s.bar->popup(), nullptr);
}

TEST(Menu, ArrowKeysOnTheBarMoveBetweenNamesAndDownOpens) {
  Scene s;
  s.root.mouseMove(Point{300, 200}, 0);
  s.root.mouseDown(s.buttonCenter(0), MouseButton::Left, 0);
  s.root.mouseUp(Point{390, 290}, MouseButton::Left, 0);
  EXPECT_EQ(s.bar->popup(), nullptr);
  s.key(kVkRight);
  s.key(kVkDown);
  EXPECT_EQ(s.bar->openMenuId(), "view");
  EXPECT_EQ(s.bar->popup()->active(), 0);
}

TEST(Menu, MenuItemTextAppendsTheMnemonicInUpperCase) {
  EXPECT_EQ(menuItemText(MenuItem{.label = "文件", .mnemonic = "f"}), "文件(F)");
  EXPECT_EQ(menuItemText(MenuItem{.label = "文件"}), "文件");
}
