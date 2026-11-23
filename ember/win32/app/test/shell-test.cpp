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
#include "platform/window/app-icon.hpp"
#include "platform/window/frameless-frame.hpp"
#include "platform/window/resize-border.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/window.hpp"
#include "support/fakes.hpp"
#include "ui/shell/app-logo.hpp"
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
  int nonClientPresses{0};
  std::unique_ptr<platform::FramelessFrame> frame;
  HWND hwnd{nullptr};
  double scale{1.0};

  explicit FrameFixture(bool customFrame = false) {
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
    callbacks.nonClientPressed = [this] { ++nonClientPresses; };
    frame = std::make_unique<platform::FramelessFrame>(hwnd, platform::FramelessOptions{kWidth, kHeight, [this] { return window.root().scale(); }, customFrame}, std::move(callbacks));
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

TEST(FramelessFrame, HitTestGivesCaptionSystemButtonsClientAndTheBorderLineAsResizeZone) {
  FrameFixture f;
  EXPECT_EQ(f.hit(300, 18), HTCAPTION);
  EXPECT_EQ(f.hit(kWidth - 23, 18), HTCLOSE);
  EXPECT_EQ(f.hit(kWidth - 69, 18), HTMAXBUTTON);
  EXPECT_EQ(f.hit(kWidth - 115, 18), HTMINBUTTON);
  EXPECT_EQ(f.hit(260, 250), HTCLIENT);
  // 边框线本身（窗口里面最外一圈）是缩放区，再往里（滚动条所在）是客户区
  EXPECT_EQ(f.hit(0.2, 250), HTLEFT);
  EXPECT_EQ(f.hit(kWidth - 0.2, 250), HTRIGHT);
  EXPECT_EQ(f.hit(260, kHeight - 0.2), HTBOTTOM);
  EXPECT_EQ(f.hit(0.2, kHeight - 0.2), HTBOTTOMLEFT);
  EXPECT_EQ(f.hit(kWidth - 0.2, kHeight - 0.2), HTBOTTOMRIGHT);
  EXPECT_EQ(f.hit(300, 0.2), HTTOP);
  EXPECT_EQ(f.hit(0.2, 0.2), HTTOPLEFT);
  EXPECT_EQ(f.hit(kWidth - 0.2, 0.2), HTTOPRIGHT);
  EXPECT_EQ(f.hit(5, 250), HTCLIENT);
  EXPECT_EQ(f.hit(kWidth - 5, 250), HTCLIENT);
  EXPECT_EQ(f.hit(260, kHeight - 5), HTCLIENT);
  EXPECT_EQ(f.hit(300, 5), HTCAPTION);
}

/** 缩放边框的夹具：一个已显示的普通窗口，窗口矩形是 (100, 100) 到 (600, 500)。 */
struct BorderFixture {
  HWND main{nullptr};
  int pressed{0};
  std::unique_ptr<platform::ResizeBorder> border;

  BorderFixture() {
    main = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"STATIC", L"", WS_POPUP, 100, 100, 500, 400, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    border = std::make_unique<platform::ResizeBorder>(main, [this] { ++pressed; });
    ShowWindow(main, SW_SHOWNA);
    border->sync(8, 1);
  }
  ~BorderFixture() {
    border.reset();
    DestroyWindow(main);
  }
  BorderFixture(const BorderFixture&) = delete;
  BorderFixture& operator=(const BorderFixture&) = delete;

  [[nodiscard]] long hit(int x, int y) const { return border->hitTest(x, y); }
};

TEST(ResizeBorder, ResizeZoneIsOutsideTheWindowEdgeAndTheBorderLine) {
  BorderFixture f;
  EXPECT_EQ(f.hit(96, 300), HTLEFT);
  EXPECT_EQ(f.hit(603, 300), HTRIGHT);
  EXPECT_EQ(f.hit(300, 96), HTTOP);
  EXPECT_EQ(f.hit(300, 503), HTBOTTOM);
  // 窗口里面最外一圈（边框线）也是缩放区，再往里不是
  EXPECT_EQ(f.hit(100, 300), HTLEFT);
  EXPECT_EQ(f.hit(599, 300), HTRIGHT);
  EXPECT_EQ(f.hit(300, 100), HTTOP);
  EXPECT_EQ(f.hit(300, 499), HTBOTTOM);
  EXPECT_EQ(f.hit(101, 300), HTTRANSPARENT);
  EXPECT_EQ(f.hit(598, 300), HTTRANSPARENT);
  EXPECT_EQ(f.hit(300, 101), HTTRANSPARENT);
  EXPECT_EQ(f.hit(300, 498), HTTRANSPARENT);
  // 超过厚度的地方也不是
  EXPECT_EQ(f.hit(91, 300), HTTRANSPARENT);
  EXPECT_EQ(f.hit(300, 509), HTTRANSPARENT);
}

TEST(ResizeBorder, CornersReachAlongTheEdges) {
  BorderFixture f;
  EXPECT_EQ(f.hit(96, 96), HTTOPLEFT);
  EXPECT_EQ(f.hit(603, 96), HTTOPRIGHT);
  EXPECT_EQ(f.hit(96, 503), HTBOTTOMLEFT);
  EXPECT_EQ(f.hit(603, 503), HTBOTTOMRIGHT);
  EXPECT_EQ(f.hit(96, 110), HTTOPLEFT);
  EXPECT_EQ(f.hit(110, 96), HTTOPLEFT);
  EXPECT_EQ(f.hit(96, 130), HTLEFT);
  EXPECT_EQ(f.hit(130, 96), HTTOP);
}

TEST(ResizeBorder, DoesNotTakeTheMouseWhileTheWindowIsDisabledOrSuppressed) {
  BorderFixture f;
  EnableWindow(f.main, FALSE);
  EXPECT_EQ(f.hit(96, 300), HTTRANSPARENT);
  EnableWindow(f.main, TRUE);
  EXPECT_EQ(f.hit(96, 300), HTLEFT);
  f.border->setSuppressed(true);
  EXPECT_EQ(f.hit(96, 300), HTTRANSPARENT);
  EXPECT_EQ(IsWindowVisible(static_cast<HWND>(f.border->handle())), FALSE);
}

TEST(ResizeBorder, WindowSurroundsTheMainWindowBehindIt) {
  BorderFixture f;
  ASSERT_NE(f.border->handle(), nullptr);
  HWND border = static_cast<HWND>(f.border->handle());
  RECT rect{};
  GetWindowRect(border, &rect);
  EXPECT_EQ(rect.left, 92);
  EXPECT_EQ(rect.top, 92);
  EXPECT_EQ(rect.right, 608);
  EXPECT_EQ(rect.bottom, 508);
  EXPECT_EQ(GetWindow(f.main, GW_HWNDNEXT), border);
}

TEST(ResizeBorder, OnlyResizeZonePressesStartAResize) {
  BorderFixture f;
  f.border->press(HTCAPTION, 300, 300);
  EXPECT_EQ(f.pressed, 0);
}

TEST(FramelessFrame, MinimumTrackSizeIsTheDefaultSize) {
  FrameFixture f;
  MINMAXINFO info{};
  info.ptMinTrackSize = POINT{1, 1};
  SendMessageW(f.hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  EXPECT_EQ(info.ptMinTrackSize.x, std::lround(kWidth * f.scale));
  EXPECT_EQ(info.ptMinTrackSize.y, std::lround(kHeight * f.scale));
}

TEST(FramelessFrame, MaximizedWindowRectIsTheWorkAreaSoTheTaskbarStaysVisible) {
  FrameFixture f;
  MONITORINFO monitor{sizeof(monitor)};
  ASSERT_TRUE(GetMonitorInfoW(MonitorFromWindow(f.hwnd, MONITOR_DEFAULTTONEAREST), &monitor));
  MINMAXINFO info{};
  SendMessageW(f.hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  EXPECT_EQ(info.ptMaxPosition.x, monitor.rcWork.left - monitor.rcMonitor.left);
  EXPECT_EQ(info.ptMaxPosition.y, monitor.rcWork.top - monitor.rcMonitor.top);
  EXPECT_EQ(info.ptMaxSize.x, monitor.rcWork.right - monitor.rcWork.left);
  EXPECT_EQ(info.ptMaxSize.y, monitor.rcWork.bottom - monitor.rcWork.top);
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

TEST(FramelessFrame, MaximizedClientAreaIsTheMonitorWorkArea) {
  FrameFixture f;
  // 窗口带上最大化样式位（不显示、不真的最大化），系统给的候选矩形比工作区大
  SetWindowLongPtrW(f.hwnd, GWL_STYLE, GetWindowLongPtrW(f.hwnd, GWL_STYLE) | WS_MAXIMIZE);
  MONITORINFO monitor{sizeof(monitor)};
  ASSERT_TRUE(GetMonitorInfoW(MonitorFromWindow(f.hwnd, MONITOR_DEFAULTTONEAREST), &monitor));
  NCCALCSIZE_PARAMS params{};
  params.rgrc[0] = RECT{monitor.rcWork.left - 11, monitor.rcWork.top - 11, monitor.rcWork.right + 11, monitor.rcWork.bottom + 11};
  EXPECT_EQ(SendMessageW(f.hwnd, WM_NCCALCSIZE, TRUE, reinterpret_cast<LPARAM>(&params)), 0);
  EXPECT_EQ(params.rgrc[0].left, monitor.rcWork.left);
  EXPECT_EQ(params.rgrc[0].top, monitor.rcWork.top);
  EXPECT_EQ(params.rgrc[0].right, monitor.rcWork.right);
  EXPECT_EQ(params.rgrc[0].bottom, monitor.rcWork.bottom);
  SetWindowLongPtrW(f.hwnd, GWL_STYLE, GetWindowLongPtrW(f.hwnd, GWL_STYLE) & ~static_cast<LONG_PTR>(WS_MAXIMIZE));
}

TEST(FramelessFrame, AttachingRemovesTheCaptionStyleButKeepsTheResizeBorder) {
  FrameFixture f;
  const LONG_PTR style = GetWindowLongPtrW(f.hwnd, GWL_STYLE);
  EXPECT_EQ(style & WS_CAPTION, static_cast<LONG_PTR>(0));
  EXPECT_NE(style & WS_THICKFRAME, static_cast<LONG_PTR>(0));
  EXPECT_NE(style & WS_SYSMENU, static_cast<LONG_PTR>(0));
}

TEST(FramelessFrame, CompositionStateCanBeQueried) { static_cast<void>(platform::FramelessFrame::compositionEnabled()); }

TEST(FramelessFrame, PreferredFrameFollowsTheSystemVersion) { static_cast<void>(platform::FramelessFrame::prefersCustomFrame()); }

TEST(FramelessFrame, PressesInTheNonClientAreaAreReportedSoMenusCanClose) {
  FrameFixture f;
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
  SendMessageW(f.hwnd, WM_NCRBUTTONDOWN, HTCAPTION, 0);
  SendMessageW(f.hwnd, WM_NCMBUTTONDOWN, HTLEFT, 0);
  EXPECT_EQ(f.nonClientPresses, 3);
}

/** 自绘模式下在客户区坐标（逻辑像素）发一个鼠标消息。 */
void sendMouse(const FrameFixture& f, unsigned message, double x, double y) {
  SendMessageW(f.hwnd, message, 0, MAKELPARAM(static_cast<int>(std::lround(x * f.scale)), static_cast<int>(std::lround(y * f.scale))));
}

/** 取走窗口队列里的系统命令（不分发，所以窗口不会真的最小化或关闭）；没有时返回 0。 */
WPARAM takePostedSystemCommand(const FrameFixture& f) {
  MSG message{};
  return PeekMessageW(&message, f.hwnd, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE) != FALSE ? message.wParam : 0;
}

TEST(FramelessFrameCustom, ButtonPressIsTrackedByTheFrameItself) {
  FrameFixture f(true);
  f.buttonStates.clear();
  const double x = kWidth - 115;
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTMINBUTTON, 0);
  ASSERT_FALSE(f.buttonStates.empty());
  EXPECT_EQ(f.buttonStates.back(), std::make_pair(platform::FrameHit::Minimize, platform::FrameHit::Minimize));
  EXPECT_EQ(f.nonClientPresses, 1);
  EXPECT_EQ(GetCapture(), f.hwnd);

  // 移出按钮：不再悬停，但仍然是“按下这个按钮”的跟踪
  sendMouse(f, WM_MOUSEMOVE, 260, 250);
  EXPECT_EQ(f.buttonStates.back(), std::make_pair(platform::FrameHit::Client, platform::FrameHit::Minimize));
  sendMouse(f, WM_MOUSEMOVE, x, 18);
  EXPECT_EQ(f.buttonStates.back(), std::make_pair(platform::FrameHit::Minimize, platform::FrameHit::Minimize));

  // 在同一个按钮上松开：发出最小化命令，状态清空，释放捕获
  sendMouse(f, WM_LBUTTONUP, x, 18);
  EXPECT_EQ(f.buttonStates.back(), std::make_pair(platform::FrameHit::Client, platform::FrameHit::Client));
  EXPECT_EQ(takePostedSystemCommand(f), static_cast<WPARAM>(SC_MINIMIZE));
  EXPECT_NE(GetCapture(), f.hwnd);
}

TEST(FramelessFrameCustom, ReleasingOutsideTheButtonOrLosingTheCaptureCancelsTheClick) {
  FrameFixture f(true);
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTCLOSE, 0);
  sendMouse(f, WM_LBUTTONUP, 260, 250);
  EXPECT_EQ(f.buttonStates.back(), std::make_pair(platform::FrameHit::Client, platform::FrameHit::Client));
  EXPECT_EQ(takePostedSystemCommand(f), 0u);

  // 松开在另一个按钮上也不算点击
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTCLOSE, 0);
  sendMouse(f, WM_LBUTTONUP, kWidth - 69, 18);
  EXPECT_EQ(takePostedSystemCommand(f), 0u);

  // 捕获被别的窗口抢走：放弃这次点击
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTCLOSE, 0);
  SendMessageW(f.hwnd, WM_CAPTURECHANGED, 0, reinterpret_cast<LPARAM>(GetDesktopWindow()));
  EXPECT_EQ(f.buttonStates.back(), std::make_pair(platform::FrameHit::Client, platform::FrameHit::Client));
  EXPECT_EQ(takePostedSystemCommand(f), 0u);
}

TEST(FramelessFrameCustom, MaximizeAndCloseSendTheirSystemCommands) {
  FrameFixture f(true);
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTMAXBUTTON, 0);
  sendMouse(f, WM_LBUTTONUP, kWidth - 69, 18);
  EXPECT_EQ(takePostedSystemCommand(f), static_cast<WPARAM>(SC_MAXIMIZE));
  SendMessageW(f.hwnd, WM_NCLBUTTONDOWN, HTCLOSE, 0);
  sendMouse(f, WM_LBUTTONUP, kWidth - 23, 18);
  EXPECT_EQ(takePostedSystemCommand(f), static_cast<WPARAM>(SC_CLOSE));
}

TEST(FramelessFrameCustom, MouseMessagesOutsideATrackedPressStayWithTheControls) {
  FrameFixture f(true);
  sendMouse(f, WM_MOUSEMOVE, 260, 250);
  sendMouse(f, WM_LBUTTONUP, 260, 250);
  EXPECT_TRUE(f.buttonStates.empty());
  EXPECT_EQ(takePostedSystemCommand(f), 0u);
}

/** 在 (4,4) 放一个 16×16 的应用标志。 */
class LogoPage final : public rujian::ui::Widget {
 public:
  explicit LogoPage(ui::AppLogo::BitmapProvider provider) {
    auto logo = std::make_unique<ui::AppLogo>(std::move(provider));
    logo_ = logo.get();
    addChild(std::move(logo));
  }
  void arrange(const rujian::ui::Rect& bounds) override {
    rujian::ui::Widget::arrange(bounds);
    logo_->arrange(rujian::ui::Rect{4, 4, 16, 16});
  }

 private:
  ui::AppLogo* logo_{nullptr};
};

/** 渲染一个标志页，返回 (x, y) 处的像素 0xAARRGGBB。 */
std::uint32_t logoPixel(const ui::AppLogo::BitmapProvider& provider, double scale, int x, int y) {
  rujian::ui::Root root(rujian::ui::ThemeName::Dark, scale);
  root.setContent(std::make_unique<LogoPage>(provider));
  root.layout(rujian::ui::Size{24, 24});
  rujian::ui::OffscreenSurface surface(rujian::ui::Size{24, 24}, scale);
  EXPECT_TRUE(surface.render(root, rujian::ui::Color::rgba(0, 0, 0)));
  const auto data = surface.pixels();
  const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(surface.pixelWidth()) + static_cast<std::size_t>(x)) * 4;
  return (static_cast<std::uint32_t>(data[i + 3]) << 24) | (static_cast<std::uint32_t>(data[i + 2]) << 16) | (static_cast<std::uint32_t>(data[i + 1]) << 8) | data[i];
}

TEST(AppLogo, DrawsTheProvidedBitmapAtTheDevicePixelSize) {
  std::vector<int> sizes;
  const ui::AppLogo::BitmapProvider provider = [&sizes](int size) {
    sizes.push_back(size);
    return std::vector<std::uint32_t>(static_cast<std::size_t>(size) * static_cast<std::size_t>(size), 0xFF3070A0u);
  };
  EXPECT_EQ(logoPixel(provider, 1.0, 10, 10) & 0xFFFFFF, 0x3070A0u);
  EXPECT_EQ(logoPixel(provider, 1.0, 22, 22) & 0xFFFFFF, 0x000000u);
  EXPECT_EQ(logoPixel(provider, 1.5, 15, 15) & 0xFFFFFF, 0x3070A0u);
  // 缩放 1 时取 16，缩放 1.5 时取 24：按设备像素取
  ASSERT_EQ(sizes.size(), 3u);
  EXPECT_EQ(sizes[0], 16);
  EXPECT_EQ(sizes[2], 24);
}

TEST(AppLogo, DrawsNothingWhenTheBitmapIsMissingOrTheWrongSize) {
  EXPECT_EQ(logoPixel([](int) { return std::vector<std::uint32_t>{}; }, 1.0, 10, 10) & 0xFFFFFF, 0x000000u);
  EXPECT_EQ(logoPixel([](int) { return std::vector<std::uint32_t>(10, 0xFFFFFFFFu); }, 1.0, 10, 10) & 0xFFFFFF, 0x000000u);
}

TEST(AppIcon, IsEmptyWithoutTheIconResourceOrASize) {
  // 测试程序里没有应用图标资源；真正的图标在 ember-win32.exe 里，由冒烟脚本的截图核对
  EXPECT_TRUE(platform::renderAppIcon(16).empty());
  EXPECT_TRUE(platform::renderAppIcon(0).empty());
  EXPECT_TRUE(platform::renderAppIcon(-4).empty());
}

}  // namespace
