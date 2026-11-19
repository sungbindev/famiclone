/*
 * ------------------------------------------------------------------------
 * 名称：app-frame-test.cpp
 * 说明：窗口框架的测试：标题栏、页面区、底部面板、状态栏自上而下的排布，底部面板高度的限制，状态栏尺寸，通知让出状态栏的高度，失焦状态的传递，页面区与底部面板的底色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：窗口 600×400。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <memory>

#include "base/theme-tokens.hpp"
#include "rujian/ui/app-frame.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"

using namespace rujian::ui;

namespace {

constexpr Size kWindow{600, 400};

struct Scene {
  Root root{ThemeName::Dark, 1.0};
  AppFrame* frame{nullptr};
  Widget* pageContent{nullptr};
  Widget* panelContent{nullptr};

  explicit Scene(bool withStatusBar = true) {
    auto appFrame = std::make_unique<AppFrame>();
    frame = appFrame.get();
    root.setContent(std::move(appFrame));
    frame->setTitleBar(std::make_unique<TitleBar>());
    if (withStatusBar) frame->setStatusBar(std::make_unique<StatusBar>());
    auto page = std::make_unique<Widget>();
    pageContent = page.get();
    frame->setPage(std::move(page));
    auto panel = std::make_unique<Widget>();
    panelContent = panel.get();
    frame->setBottomPanel(std::move(panel));
    root.layout(kWindow);
  }
};

}  // namespace

TEST(AppFrame, StacksTitleBarPageAndStatusBarTopToBottom) {
  Scene s;
  EXPECT_DOUBLE_EQ(s.frame->titleBar()->bounds().y, 0.0);
  EXPECT_DOUBLE_EQ(s.frame->titleBar()->bounds().height, 36.0);
  EXPECT_DOUBLE_EQ(s.frame->statusBar()->bounds().height, 26.0);
  EXPECT_DOUBLE_EQ(s.frame->statusBar()->bounds().bottom(), 400.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().y, 36.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().bottom(), 400.0 - 26.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().width, 600.0);
}

TEST(AppFrame, LargeStatusBarIsTallerAndTheRegionsFollow) {
  Scene s;
  s.frame->setStatusBarLarge(true);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->statusBar()->bounds().height, 32.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().bottom(), 400.0 - 32.0);
}

TEST(AppFrame, HiddenTitleBarAndStatusBarGiveTheirSpaceToThePage) {
  Scene s;
  s.frame->titleBar()->setVisible(false);
  s.frame->statusBar()->setVisible(false);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().y, 0.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().bottom(), 400.0);
  s.frame->titleBar()->setVisible(true);
  s.frame->statusBar()->setVisible(true);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().y, 36.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().bottom(), 400.0 - 26.0);
}

TEST(AppFrame, BottomPanelTakesItsHeightFromThePageAreaWhenOpen) {
  Scene s;
  EXPECT_FALSE(s.frame->bottomRegion()->visible());
  s.frame->setBottomPanelMinHeight(60);
  s.frame->setBottomPanelHeight(100);
  s.frame->setBottomPanelOpen(true);
  s.root.layout(kWindow);
  const Rect panel = s.frame->bottomRegion()->bounds();
  EXPECT_DOUBLE_EQ(panel.height, 100.0);
  EXPECT_DOUBLE_EQ(panel.bottom(), 400.0 - 26.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().bottom(), panel.y);
}

TEST(AppFrame, BottomPanelHeightIsClampedBetweenTheMinimumAndSeventyPercentOfTheWindow) {
  Scene s;
  s.frame->setBottomPanelOpen(true);
  s.frame->setBottomPanelHeight(10);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->shownBottomHeight(), 120.0);
  s.frame->setBottomPanelHeight(1000);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->shownBottomHeight(), 280.0);
  s.frame->setBottomPanelMinHeight(60);
  s.frame->setBottomPanelHeight(40);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->shownBottomHeight(), 60.0);
}

TEST(AppFrame, ClosedBottomPanelTakesNoSpace) {
  Scene s;
  s.frame->setBottomPanelOpen(true);
  s.frame->setBottomPanelOpen(false);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.frame->shownBottomHeight(), 0.0);
  EXPECT_DOUBLE_EQ(s.frame->pageRegion()->bounds().bottom(), 400.0 - 26.0);
}

TEST(AppFrame, ContentsFillTheirRegionsAndThePanelContentSitsBelowTheBorder) {
  Scene s;
  s.frame->setBottomPanelOpen(true);
  s.frame->setBottomPanelMinHeight(60);
  s.frame->setBottomPanelHeight(100);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(s.pageContent->bounds().height, s.frame->pageRegion()->bounds().height);
  EXPECT_DOUBLE_EQ(s.panelContent->bounds().y, s.frame->bottomRegion()->bounds().y + 1.0);
  EXPECT_DOUBLE_EQ(s.panelContent->bounds().height, 99.0);
}

TEST(AppFrame, NotificationsMakeRoomForTheStatusBar) {
  Scene s;
  NotificationCenter center(s.root);
  s.frame->setNotificationCenter(&center);
  s.root.layout(kWindow);
  center.notify("一条通知");
  EXPECT_DOUBLE_EQ(center.toastRect(0).bottom(), 400.0 - (26.0 + 6.0));
  s.frame->setStatusBarLarge(true);
  s.root.layout(kWindow);
  EXPECT_DOUBLE_EQ(center.toastRect(0).bottom(), 400.0 - (32.0 + 6.0));
  s.frame->setNotificationCenter(nullptr);
}

TEST(AppFrame, WithoutAStatusBarNotificationsKeepTheDefaultOffset) {
  Scene s(false);
  NotificationCenter center(s.root);
  s.frame->setNotificationCenter(&center);
  s.root.layout(kWindow);
  center.notify("一条通知");
  EXPECT_DOUBLE_EQ(center.toastRect(0).bottom(), 400.0 - 10.0);
  s.frame->setNotificationCenter(nullptr);
}

namespace {

std::vector<std::uint8_t> render(Scene& s) {
  OffscreenSurface surface(kWindow, 1.0);
  EXPECT_TRUE(surface.render(s.root, Color::rgba(0, 0, 0)));
  return surface.pixels();
}

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

TEST(AppFrame, PageAndBottomPanelUseTheirOwnBackgrounds) {
  Scene s;
  s.frame->setBottomPanelOpen(true);
  s.frame->setBottomPanelHeight(100);
  s.root.layout(kWindow);
  const auto pixels = render(s);
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(pixels, 300, 100), compose(colors.content_bg, Color::rgba(0, 0, 0)));
  expectColor(pixelAt(pixels, 300, 400 - 26 - 50), compose(colors.chrome_bg, Color::rgba(0, 0, 0)));
}

TEST(AppFrame, WindowInactiveReachesTheTitleBarAndTheStatusBar) {
  Scene s;
  s.frame->setWindowInactive(true);
  const auto pixels = render(s);
  const auto& colors = themeTokens(ThemeName::Dark);
  expectColor(pixelAt(pixels, 300, 5), compose(colors.title_bar_inactive_bg, Color::rgba(0, 0, 0)));
  expectColor(pixelAt(pixels, 300, 400 - 10), compose(colors.status_bar_inactive_bg, Color::rgba(0, 0, 0)));
}
