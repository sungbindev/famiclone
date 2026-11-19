/*
 * ------------------------------------------------------------------------
 * 名称：title-bar-test.cpp
 * 说明：标题栏的测试：各部分的位置、折叠按钮与操作区靠右排列、系统按钮的区域与命中测试、点击折叠按钮的回调、菜单栏嵌在标题栏里仍可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口宽 600、高 36，标题栏铺满窗口。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/title-bar.hpp"

using namespace rujian::ui;

namespace {

constexpr Size kWindow{600, 36};

/** 16×16 的占位图标。 */
class Logo final : public Widget {};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  TitleBar* bar{nullptr};
  Button* update{nullptr};
  std::vector<std::string> clicked;

  explicit Scene(bool windowButtons = true) {
    auto titleBar = std::make_unique<TitleBar>();
    bar = titleBar.get();
    root.setContent(std::move(titleBar));
    bar->setLogo(std::make_unique<Logo>());
    bar->setTitle("Ember");
    bar->setSubtitle("模拟器");
    auto menus = std::make_unique<MenuBar>();
    menus->setMenus({MenuBarMenu{.id = "file", .label = "文件", .mnemonic = "F", .items = {MenuItem{.id = "open", .label = "打开"}}}});
    bar->setMenuBar(std::move(menus));
    auto button = std::make_unique<Button>("更新");
    update = static_cast<Button*>(bar->addAction(std::move(button)));
    bar->setTools({TitleBarTool{.id = "side", .label = "边栏", .icon = IconId::Settings, .expanded = true},
                   TitleBarTool{.id = "bottom", .label = "底部面板", .icon = IconId::Eye}});
    bar->setOnToolClick([this](const std::string& id) { clicked.push_back(id); });
    bar->setWindowButtons(windowButtons);
    root.layout(kWindow);
  }

  void click(Point point) {
    root.mouseMove(point, 0);
    root.mouseDown(point, MouseButton::Left, 0);
    root.mouseUp(point, MouseButton::Left, 0);
  }

  static Point center(const Rect& rect) { return Point{rect.x + rect.width / 2, rect.y + rect.height / 2}; }
};

}  // namespace

TEST(TitleBar, HasTheTokenHeightAndCentersItsPartsInTheContentArea) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.bar->bounds().height, 36.0);
  const double centerY = (36.0 - 1.0) / 2.0;
  const Rect menus = s.bar->menuBar()->bounds();
  EXPECT_DOUBLE_EQ(menus.height, 24.0);
  EXPECT_DOUBLE_EQ(menus.y + menus.height / 2, centerY);
  EXPECT_DOUBLE_EQ(s.bar->children().front()->bounds().x, 8.0);
}

TEST(TitleBar, MenuBarFollowsTheBrandWithAGap) {
  Scene s;
  const Rect logo = s.bar->children().front()->bounds();
  EXPECT_DOUBLE_EQ(logo.width, 16.0);
  EXPECT_DOUBLE_EQ(logo.height, 16.0);
  EXPECT_GT(s.bar->menuBar()->bounds().x, logo.right() + 6.0);
}

TEST(TitleBar, ToolsAreRightAlignedLeftOfTheSystemButtons) {
  Scene s;
  const Rect last = s.bar->toolButton(1)->bounds();
  const Rect first = s.bar->toolButton(0)->bounds();
  EXPECT_DOUBLE_EQ(last.width, 46.0);
  EXPECT_DOUBLE_EQ(last.height, 35.0);
  EXPECT_DOUBLE_EQ(last.right(), 600.0 - 3 * 46.0 - 10.0);
  EXPECT_DOUBLE_EQ(first.right(), last.x);
}

TEST(TitleBar, ToolsMoveToTheRightEdgeWithoutSystemButtons) {
  Scene s(false);
  EXPECT_DOUBLE_EQ(s.bar->toolButton(1)->bounds().right(), 600.0 - 10.0);
}

TEST(TitleBar, ActionsSitLeftOfTheToolsWithAGap) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.update->bounds().right(), s.bar->toolButton(0)->bounds().x - 10.0);
}

TEST(TitleBar, SystemButtonsAre46WideAndFillTheFullHeight) {
  Scene s;
  const Rect close = s.bar->systemButtonRect(TitleBarHit::Close);
  const Rect maximize = s.bar->systemButtonRect(TitleBarHit::Maximize);
  const Rect minimize = s.bar->systemButtonRect(TitleBarHit::Minimize);
  EXPECT_DOUBLE_EQ(close.right(), 600.0);
  EXPECT_DOUBLE_EQ(close.width, 46.0);
  EXPECT_DOUBLE_EQ(close.height, 36.0);
  EXPECT_DOUBLE_EQ(maximize.right(), close.x);
  EXPECT_DOUBLE_EQ(minimize.right(), maximize.x);
}

TEST(TitleBar, NoSystemButtonAreasWhenTheyAreNotShown) {
  Scene s(false);
  EXPECT_DOUBLE_EQ(s.bar->systemButtonRect(TitleBarHit::Close).width, 0.0);
  EXPECT_EQ(s.bar->hitTestWindow(Point{590, 10}), TitleBarHit::Caption);
}

TEST(TitleBar, HitTestReturnsSystemButtonsCaptionAndClient) {
  Scene s;
  EXPECT_EQ(s.bar->hitTestWindow(Scene::center(s.bar->systemButtonRect(TitleBarHit::Close))), TitleBarHit::Close);
  EXPECT_EQ(s.bar->hitTestWindow(Scene::center(s.bar->systemButtonRect(TitleBarHit::Maximize))), TitleBarHit::Maximize);
  EXPECT_EQ(s.bar->hitTestWindow(Scene::center(s.bar->systemButtonRect(TitleBarHit::Minimize))), TitleBarHit::Minimize);
  EXPECT_EQ(s.bar->hitTestWindow(Scene::center(s.bar->menuBar()->bounds())), TitleBarHit::Client);
  EXPECT_EQ(s.bar->hitTestWindow(Scene::center(s.bar->toolButton(0)->bounds())), TitleBarHit::Client);
  EXPECT_EQ(s.bar->hitTestWindow(Scene::center(s.update->bounds())), TitleBarHit::Client);
  // 菜单栏与操作区之间的空白处可以拖动窗口
  const double gapX = (s.bar->menuBar()->bounds().right() + s.update->bounds().x) / 2.0;
  EXPECT_EQ(s.bar->hitTestWindow(Point{gapX, 18}), TitleBarHit::Caption);
  EXPECT_EQ(s.bar->hitTestWindow(Point{300, 100}), TitleBarHit::Client);
}

TEST(TitleBar, ClickingAToolReportsItsId) {
  Scene s;
  s.click(Scene::center(s.bar->toolButton(1)->bounds()));
  s.click(Scene::center(s.bar->toolButton(0)->bounds()));
  EXPECT_EQ(s.clicked, (std::vector<std::string>{"bottom", "side"}));
}

TEST(TitleBar, DisabledToolsDoNotReportClicks) {
  Scene s;
  s.bar->setTools({TitleBarTool{.id = "side", .label = "边栏", .icon = IconId::Settings, .disabled = true}});
  s.root.layout(kWindow);
  s.click(Scene::center(s.bar->toolButton(0)->bounds()));
  EXPECT_TRUE(s.clicked.empty());
}

TEST(TitleBar, MenuBarInsideStillOpensItsMenu) {
  Scene s;
  s.click(Scene::center(s.bar->menuBar()->buttonRect(0)));
  EXPECT_NE(s.bar->menuBar()->popup(), nullptr);
  EXPECT_DOUBLE_EQ(s.bar->menuBar()->popup()->bounds().y, s.bar->menuBar()->buttonRect(0).bottom() + 2.0);
}

TEST(TitleBar, ReplacingToolsRebuildsTheButtons) {
  Scene s;
  s.bar->setTools({TitleBarTool{.id = "only", .label = "仅有", .icon = IconId::Settings}});
  s.root.layout(kWindow);
  EXPECT_NE(s.bar->toolButton(0), nullptr);
  EXPECT_EQ(s.bar->toolButton(1), nullptr);
  EXPECT_DOUBLE_EQ(s.bar->toolButton(0)->bounds().right(), 600.0 - 3 * 46.0 - 10.0);
}

TEST(TitleBar, SystemButtonStateAndMaximizedFlagDoNotChangeTheLayout) {
  Scene s;
  const Rect before = s.bar->systemButtonRect(TitleBarHit::Maximize);
  s.bar->setSystemButtonState(TitleBarHit::Maximize, TitleBarHit::Client);
  s.bar->setMaximized(true);
  s.bar->setWindowActive(false);
  EXPECT_DOUBLE_EQ(s.bar->systemButtonRect(TitleBarHit::Maximize).x, before.x);
}

namespace {

/** 渲染场景，返回像素。 */
std::vector<std::uint8_t> render(Scene& s) {
  OffscreenSurface surface(kWindow, 1.0);
  EXPECT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  return surface.pixels();
}

/** 取像素的 ARGB 值。 */
std::uint32_t pixelAt(const std::vector<std::uint8_t>& data, int x, int y) {
  const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWindow.width) + static_cast<std::size_t>(x)) * 4;
  return (static_cast<std::uint32_t>(data[i + 3]) << 24) | (static_cast<std::uint32_t>(data[i + 2]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) | data[i];
}

void expectColor(std::uint32_t actual, Color expected) {
  EXPECT_NEAR(static_cast<int>((actual >> 16) & 0xFF), expected.r(), 1);
  EXPECT_NEAR(static_cast<int>((actual >> 8) & 0xFF), expected.g(), 1);
  EXPECT_NEAR(static_cast<int>(actual & 0xFF), expected.b(), 1);
}

}  // namespace

TEST(TitleBar, PaintsTheBackgroundAndTheCloseButtonRedWhenHovered) {
  Scene s;
  const Color background = compose(themeTokens(ThemeName::Dark).title_bar_bg, Color::rgba(0, 0, 0));
  const Rect close = s.bar->systemButtonRect(TitleBarHit::Close);
  auto pixels = render(s);
  expectColor(pixelAt(pixels, 300, 5), background);
  expectColor(pixelAt(pixels, static_cast<int>(close.x) + 2, 2), background);
  s.bar->setSystemButtonState(TitleBarHit::Close, TitleBarHit::Client);
  pixels = render(s);
  expectColor(pixelAt(pixels, static_cast<int>(close.x) + 2, 2), Color::rgba(0xC4, 0x2B, 0x1C));
  // 关闭按钮的底色不盖住底部分割线所在的那一行，分割线在系统按钮下面也是连续的
  expectColor(pixelAt(pixels, static_cast<int>(close.x) + 2, 34), Color::rgba(0xC4, 0x2B, 0x1C));
  const Color line = compose(themeTokens(ThemeName::Dark).chrome_border, background);
  expectColor(pixelAt(pixels, static_cast<int>(close.x) + 2, 35), line);
  expectColor(pixelAt(pixels, 300, 35), line);
}

TEST(TitleBar, InactiveWindowUsesTheInactiveBackground) {
  Scene s;
  s.bar->setWindowActive(false);
  const auto pixels = render(s);
  expectColor(pixelAt(pixels, 300, 5), compose(themeTokens(ThemeName::Dark).title_bar_inactive_bg, Color::rgba(0, 0, 0)));
}

TEST(TitleBar, MaximizeGlyphBecomesTheRestoreGlyphWhenMaximized) {
  Scene s;
  const Rect box = s.bar->systemButtonRect(TitleBarHit::Maximize);
  const int left = static_cast<int>(box.x + (box.width - 10) / 2);
  const int top = static_cast<int>((box.height - 10) / 2);
  const Color background = compose(themeTokens(ThemeName::Dark).title_bar_bg, Color::rgba(0, 0, 0));
  auto pixels = render(s);
  // 最大化图形：左上角有线
  EXPECT_NE(pixelAt(pixels, left, top) & 0xFFFFFF, (static_cast<std::uint32_t>(background.r()) << 16) | (static_cast<std::uint32_t>(background.g()) << 8) | background.b());
  s.bar->setMaximized(true);
  pixels = render(s);
  // 还原图形：后面的方框从左上角缩进 2 像素，左上角是空的
  expectColor(pixelAt(pixels, left, top), background);
}

TEST(TitleBar, MinimizeGlyphIsAHorizontalLineInTheMiddle) {
  Scene s;
  const Rect box = s.bar->systemButtonRect(TitleBarHit::Minimize);
  const int x = static_cast<int>(box.x + box.width / 2);
  const int middle = static_cast<int>(box.height / 2);
  const Color background = compose(themeTokens(ThemeName::Dark).title_bar_bg, Color::rgba(0, 0, 0));
  const auto pixels = render(s);
  expectColor(pixelAt(pixels, x, middle - 4), background);
  EXPECT_NE(pixelAt(pixels, x, middle) & 0xFFFFFF, (static_cast<std::uint32_t>(background.r()) << 16) | (static_cast<std::uint32_t>(background.g()) << 8) | background.b());
}
