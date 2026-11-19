/*
 * ------------------------------------------------------------------------
 * 名称：shell-test.cpp
 * 说明：外壳界面与主题的测试：主窗口内容的布局与命中测试、主题同步、自绘标题栏窗口的非客户区行为（缩放边框、标题栏、系统按钮、最小尺寸、悬停状态）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：主窗口内容用控件库的离屏根容器测试；非客户区行为用一个不显示的真实窗口测试（向它发窗口消息，不使用鼠标与键盘）。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <windows.h>
#include <windowsx.h>

#include <cmath>

#include "entry/preferences/theme-sync.hpp"
#include "platform/window/frameless-frame.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/window.hpp"
#include "support/fakes.hpp"
#include "ui/shell/main-window-view.hpp"

namespace {

using namespace ember::win32;
using nlohmann::json;
using rujian::ui::Point;
using rujian::ui::TitleBarHit;

constexpr double kWidth = 524;
constexpr double kHeight = 500;

TEST(MainWindowView, LaysOutTitleBarDisplayAreaAndLargeStatusBar) {
  rujian::ui::Root root(rujian::ui::ThemeName::Dark, 1.0);
  auto content = std::make_unique<ui::MainWindowView>("EMBER");
  ui::MainWindowView* view = content.get();
  root.setContent(std::move(content));
  root.layout({kWidth, kHeight});

  EXPECT_EQ(view->titleBar().bounds().height, 36);
  EXPECT_EQ(view->titleBar().bounds().y, 0);
  EXPECT_EQ(view->frame().statusBar()->bounds().height, 32);
  EXPECT_EQ(view->frame().statusBar()->bounds().bottom(), kHeight);
  // 底部面板默认收起，显示区占满标题栏与系统栏之间
  EXPECT_EQ(view->frame().shownBottomHeight(), 0);
  EXPECT_EQ(view->frame().pageRegion()->bounds().y, 36);
  EXPECT_EQ(view->frame().pageRegion()->bounds().bottom(), kHeight - 32);
  EXPECT_EQ(view->frame().pageRegion()->bounds().width, kWidth);
}

TEST(MainWindowView, AnswersTheHitTestsTheWindowNeeds) {
  rujian::ui::Root root(rujian::ui::ThemeName::Dark, 1.0);
  auto content = std::make_unique<ui::MainWindowView>("EMBER");
  ui::MainWindowView* view = content.get();
  root.setContent(std::move(content));
  root.layout({kWidth, kHeight});

  EXPECT_EQ(view->hitTestWindow(Point{kWidth - 23, 18}), TitleBarHit::Close);
  EXPECT_EQ(view->hitTestWindow(Point{kWidth - 69, 18}), TitleBarHit::Maximize);
  EXPECT_EQ(view->hitTestWindow(Point{kWidth - 115, 18}), TitleBarHit::Minimize);
  EXPECT_EQ(view->hitTestWindow(Point{300, 18}), TitleBarHit::Caption);
  EXPECT_EQ(view->hitTestWindow(Point{200, 200}), TitleBarHit::Client);
}

TEST(MainWindowView, WindowStateChangesDoNotBreakTheLayout) {
  rujian::ui::Root root(rujian::ui::ThemeName::Light, 1.5);
  auto content = std::make_unique<ui::MainWindowView>("EMBER");
  ui::MainWindowView* view = content.get();
  root.setContent(std::move(content));
  root.layout({kWidth, kHeight});
  view->setMaximized(true);
  view->setWindowActive(false);
  view->setBorderVisible(true);
  view->setSystemButtonState(TitleBarHit::Close, TitleBarHit::Client);
  root.layout({800, 600});
  EXPECT_EQ(view->frame().statusBar()->bounds().bottom(), 600);
  view->setMaximized(false);
  EXPECT_EQ(view->hitTestWindow(Point{800 - 23, 18}), TitleBarHit::Close);
}

/** 一次主题同步的夹具。 */
struct ThemeFixture {
  test::FakeCoreGateway core;
  std::vector<entry::ThemeSource> applied;
  std::vector<std::string> errors;
  int firstCalls{0};

  std::unique_ptr<entry::ThemeSync> create() {
    return std::make_unique<entry::ThemeSync>(core, [this](entry::ThemeSource source) { applied.push_back(source); }, [this](const std::string& message) { errors.push_back(message); }, [this] { ++firstCalls; });
  }

  void answerPreferencesList(const std::string& theme) {
    core.replies["preferences.list"] = [theme](const json&) {
      return contracts::CoreReply::success({{"settings", {{{"id", "other.setting"}, {"value", "x"}}, {{"id", "appearance.theme"}, {"value", theme}}}}});
    };
  }
};

TEST(ThemeSync, ReadsTheThemeAtOnceWhenTheCoreIsAlreadyRunning) {
  ThemeFixture f;
  f.core.current.state = contracts::CoreState::Running;
  f.answerPreferencesList("light");
  const auto sync = f.create();
  EXPECT_EQ(f.applied, std::vector<entry::ThemeSource>{entry::ThemeSource::Light});
  EXPECT_EQ(f.firstCalls, 1);
  ASSERT_EQ(f.core.calls.size(), 1u);
  EXPECT_EQ(f.core.calls[0].name, "preferences.list");
}

TEST(ThemeSync, WaitsForTheCoreAndReadsAgainWhenItBecomesReadyAgain) {
  ThemeFixture f;
  f.answerPreferencesList("system");
  const auto sync = f.create();
  EXPECT_TRUE(f.core.calls.empty());
  EXPECT_EQ(f.firstCalls, 0);
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.applied, std::vector<entry::ThemeSource>{entry::ThemeSource::System});
  EXPECT_EQ(f.firstCalls, 1);
  f.answerPreferencesList("dark");
  f.core.setState(contracts::CoreState::Running);
  EXPECT_EQ(f.applied.back(), entry::ThemeSource::Dark);
  EXPECT_EQ(f.firstCalls, 1);
}

TEST(ThemeSync, FollowsChangeEventsForTheThemeSettingOnly) {
  ThemeFixture f;
  const auto sync = f.create();
  f.core.emitEvent("preferences.changed", {{"id", "appearance.theme"}, {"value", "dark"}});
  f.core.emitEvent("preferences.changed", {{"id", "debug.enabled"}, {"value", true}});
  f.core.emitEvent("preferences.changed", {{"id", "appearance.theme"}, {"value", "purple"}});
  f.core.emitEvent("runtime.stateChanged", {{"id", "appearance.theme"}, {"value", "light"}});
  f.core.emitEvent("preferences.changed", "text");
  EXPECT_EQ(f.applied, std::vector<entry::ThemeSource>{entry::ThemeSource::Dark});
}

TEST(ThemeSync, FailedReadIsReportedAndStillEndsTheWait) {
  ThemeFixture f;
  f.core.replies["preferences.list"] = [](const json&) { return contracts::CoreReply::failure("core.unavailable", "x"); };
  f.core.current.state = contracts::CoreState::Running;
  const auto sync = f.create();
  EXPECT_TRUE(f.applied.empty());
  ASSERT_EQ(f.errors.size(), 1u);
  EXPECT_EQ(f.firstCalls, 1);
}

TEST(ThemeSync, CoreThatCannotStartEndsTheWaitWithoutATheme) {
  ThemeFixture f;
  const auto sync = f.create();
  f.core.setState(contracts::CoreState::Failed);
  EXPECT_TRUE(f.applied.empty());
  EXPECT_EQ(f.firstCalls, 1);
}

TEST(ThemeSync, UnsubscribesWhenDestroyed) {
  ThemeFixture f;
  {
    const auto sync = f.create();
    EXPECT_EQ(f.core.subscriberCount(), 2u);
  }
  EXPECT_EQ(f.core.subscriberCount(), 0u);
  f.core.emitEvent("preferences.changed", {{"id", "appearance.theme"}, {"value", "dark"}});
  EXPECT_TRUE(f.applied.empty());
}

/** 自绘标题栏窗口的夹具：不显示的真实窗口，内容是主窗口内容。 */
struct FrameFixture {
  rujian::ui::HostWindow window{rujian::ui::HostWindow::Options{L"测试窗口", rujian::ui::Size{kWidth, kHeight}, rujian::ui::ThemeName::Dark}};
  ui::MainWindowView* view{nullptr};
  std::vector<std::pair<platform::FrameHit, platform::FrameHit>> buttonStates;
  std::vector<bool> activeStates;
  std::vector<bool> maximizedStates;
  std::vector<std::pair<int, int>> moves;
  int themeChanges{0};
  std::unique_ptr<platform::FramelessFrame> frame;
  HWND hwnd{nullptr};
  double scale{1.0};

  FrameFixture() {
    hwnd = static_cast<HWND>(window.nativeHandle());
    auto content = std::make_unique<ui::MainWindowView>("EMBER");
    view = content.get();
    window.root().setContent(std::move(content));
    scale = window.root().scale();

    platform::FramelessCallbacks callbacks;
    callbacks.hitTest = [this](double x, double y) {
      switch (view->hitTestWindow(Point{x, y})) {
        case TitleBarHit::Caption:
          return platform::FrameHit::Caption;
        case TitleBarHit::Minimize:
          return platform::FrameHit::Minimize;
        case TitleBarHit::Maximize:
          return platform::FrameHit::Maximize;
        case TitleBarHit::Close:
          return platform::FrameHit::Close;
        case TitleBarHit::Client:
          break;
      }
      return platform::FrameHit::Client;
    };
    callbacks.systemButtonState = [this](platform::FrameHit hover, platform::FrameHit pressed) { buttonStates.emplace_back(hover, pressed); };
    callbacks.activeChanged = [this](bool active) { activeStates.push_back(active); };
    callbacks.maximizedChanged = [this](bool maximized) { maximizedStates.push_back(maximized); };
    callbacks.moved = [this](int x, int y) { moves.emplace_back(x, y); };
    callbacks.systemThemeChanged = [this] { ++themeChanges; };
    frame = std::make_unique<platform::FramelessFrame>(hwnd, platform::FramelessOptions{kWidth, kHeight, [this] { return window.root().scale(); }}, std::move(callbacks));
    window.setMessageHook([this](unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) { return frame->handle(message, wParam, lParam, result); });
    frame->attach();
    SetWindowPos(hwnd, nullptr, 200, 150, static_cast<int>(std::lround(kWidth * scale)), static_cast<int>(std::lround(kHeight * scale)), SWP_NOZORDER | SWP_NOACTIVATE);
  }

  /** 窗口里一个位置（逻辑像素）的命中结果。 */
  LRESULT hit(double x, double y) const {
    RECT rect{};
    GetWindowRect(hwnd, &rect);
    return SendMessageW(hwnd, WM_NCHITTEST, 0, MAKELPARAM(static_cast<int>(rect.left + std::lround(x * scale)), static_cast<int>(rect.top + std::lround(y * scale))));
  }
};

TEST(FramelessFrame, WholeWindowBecomesTheClientArea) {
  FrameFixture f;
  RECT window{};
  RECT client{};
  GetWindowRect(f.hwnd, &window);
  GetClientRect(f.hwnd, &client);
  EXPECT_EQ(client.right - client.left, window.right - window.left);
  EXPECT_EQ(client.bottom - client.top, window.bottom - window.top);
  EXPECT_EQ(f.window.root().size().width * f.scale, window.right - window.left);
}

TEST(FramelessFrame, HitTestGivesResizeBordersCaptionSystemButtonsAndClient) {
  FrameFixture f;
  EXPECT_EQ(f.hit(300, 18), HTCAPTION);
  EXPECT_EQ(f.hit(kWidth - 23, 18), HTCLOSE);
  EXPECT_EQ(f.hit(kWidth - 69, 18), HTMAXBUTTON);
  EXPECT_EQ(f.hit(kWidth - 115, 18), HTMINBUTTON);
  EXPECT_EQ(f.hit(260, 250), HTCLIENT);
  EXPECT_EQ(f.hit(0.2, 250), HTLEFT);
  EXPECT_EQ(f.hit(kWidth - 0.2, 250), HTRIGHT);
  EXPECT_EQ(f.hit(260, kHeight - 0.2), HTBOTTOM);
  EXPECT_EQ(f.hit(0.2, kHeight - 0.2), HTBOTTOMLEFT);
  EXPECT_EQ(f.hit(kWidth - 0.2, kHeight - 0.2), HTBOTTOMRIGHT);
  EXPECT_EQ(f.hit(300, 0.2), HTTOP);
  EXPECT_EQ(f.hit(0.2, 0.2), HTTOPLEFT);
  EXPECT_EQ(f.hit(kWidth - 0.2, 0.2), HTTOPRIGHT);
}

TEST(FramelessFrame, MinimumTrackSizeIsTheDefaultSize) {
  FrameFixture f;
  MINMAXINFO info{};
  info.ptMinTrackSize = POINT{1, 1};
  SendMessageW(f.hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  EXPECT_EQ(info.ptMinTrackSize.x, std::lround(kWidth * f.scale));
  EXPECT_EQ(info.ptMinTrackSize.y, std::lround(kHeight * f.scale));
}

TEST(FramelessFrame, SystemButtonHoverIsReportedAndClearedOnLeave) {
  FrameFixture f;
  SendMessageW(f.hwnd, WM_NCMOUSEMOVE, HTCLOSE, 0);
  ASSERT_FALSE(f.buttonStates.empty());
  EXPECT_EQ(f.buttonStates.back().first, platform::FrameHit::Close);
  SendMessageW(f.hwnd, WM_NCMOUSEMOVE, HTMAXBUTTON, 0);
  EXPECT_EQ(f.buttonStates.back().first, platform::FrameHit::Maximize);
  SendMessageW(f.hwnd, WM_NCMOUSEMOVE, HTCAPTION, 0);
  EXPECT_EQ(f.buttonStates.back().first, platform::FrameHit::Client);
  SendMessageW(f.hwnd, WM_NCMOUSEMOVE, HTMINBUTTON, 0);
  SendMessageW(f.hwnd, WM_NCMOUSELEAVE, 0, 0);
  EXPECT_EQ(f.buttonStates.back().first, platform::FrameHit::Client);
}

TEST(FramelessFrame, ActivationAndThemeSettingChangesAreReported) {
  FrameFixture f;
  SendMessageW(f.hwnd, WM_NCACTIVATE, TRUE, 0);
  SendMessageW(f.hwnd, WM_NCACTIVATE, FALSE, 0);
  EXPECT_EQ(f.activeStates, (std::vector<bool>{true, false}));
  SendMessageW(f.hwnd, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"ImmersiveColorSet"));
  SendMessageW(f.hwnd, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"intl"));
  EXPECT_EQ(f.themeChanges, 1);
}

TEST(FramelessFrame, MovesAreReportedWithTheWindowPosition) {
  FrameFixture f;
  f.moves.clear();
  SetWindowPos(f.hwnd, nullptr, 321, 123, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  ASSERT_FALSE(f.moves.empty());
  EXPECT_EQ(f.moves.back(), std::make_pair(321, 123));
}

TEST(FramelessFrame, CompositionStateCanBeQueried) { static_cast<void>(platform::FramelessFrame::compositionEnabled()); }

}  // namespace
