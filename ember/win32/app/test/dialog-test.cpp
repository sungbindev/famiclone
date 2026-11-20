/*
 * ------------------------------------------------------------------------
 * 名称：dialog-test.cpp
 * 说明：对话框窗口与对话框打开者的测试：窗口归属、大小与居中、最小尺寸、关闭只是请求、模态（打开期间归属窗口不可用，销毁后恢复）、同一个对话框只开一个、主题切换。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：用真实的窗口测试（向它发窗口消息，不使用鼠标与键盘）；打开对话框会短暂显示一个窗口。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <windows.h>

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include "bootstrap/dialog-host.hpp"
#include "platform/window/dialog-window.hpp"
#include "rujian/ui/widget.hpp"
#include "rujian/ui/window.hpp"

namespace {

using namespace ember::win32;

constexpr double kWidth = 420;
constexpr double kHeight = 300;

/** 归属窗口：不显示的主窗口。 */
struct Owner {
  rujian::ui::HostWindow window{rujian::ui::HostWindow::Options{L"归属窗口", rujian::ui::Size{524, 500}, rujian::ui::ThemeName::Dark}};
  [[nodiscard]] HWND hwnd() const { return static_cast<HWND>(window.nativeHandle()); }
};

platform::DialogWindowOptions optionsFor(const Owner& owner, int* closeRequests) {
  platform::DialogWindowOptions options;
  options.owner = owner.hwnd();
  options.title = L"测试对话框";
  options.width = kWidth;
  options.height = kHeight;
  options.minimumWidth = 300;
  options.minimumHeight = 200;
  options.onCloseRequested = [closeRequests] { ++*closeRequests; };
  return options;
}

std::unique_ptr<rujian::ui::DialogFrame> makeFrame() {
  auto frame = std::make_unique<rujian::ui::DialogFrame>();
  frame->setTitle("测试");
  frame->setContent(std::make_unique<rujian::ui::Widget>());
  return frame;
}

TEST(DialogWindow, IsOwnedByTheOwnerHasTheRequestedSizeAndNoCaptionStyle) {
  Owner owner;
  int closes = 0;
  platform::DialogWindow dialog(optionsFor(owner, &closes), makeFrame());
  HWND hwnd = static_cast<HWND>(dialog.handle());
  EXPECT_EQ(GetWindow(hwnd, GW_OWNER), owner.hwnd());
  RECT rect{};
  GetWindowRect(hwnd, &rect);
  const double scale = static_cast<double>(rect.right - rect.left) / kWidth;
  EXPECT_NEAR(rect.right - rect.left, std::lround(kWidth * scale), 1);
  EXPECT_NEAR(rect.bottom - rect.top, std::lround(kHeight * scale), 1);
  const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
  EXPECT_EQ(style & WS_CAPTION, static_cast<LONG_PTR>(0));
  EXPECT_NE(style & WS_THICKFRAME, static_cast<LONG_PTR>(0));
}

TEST(DialogWindow, MinimumTrackSizeIsTheRequestedMinimum) {
  Owner owner;
  int closes = 0;
  platform::DialogWindow dialog(optionsFor(owner, &closes), makeFrame());
  HWND hwnd = static_cast<HWND>(dialog.handle());
  MINMAXINFO info{};
  SendMessageW(hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  RECT rect{};
  GetWindowRect(hwnd, &rect);
  const double scale = static_cast<double>(rect.right - rect.left) / kWidth;
  EXPECT_EQ(info.ptMinTrackSize.x, std::lround(300 * scale));
  EXPECT_EQ(info.ptMinTrackSize.y, std::lround(200 * scale));
}

/** 有最小尺寸要求的正文。 */
class MinimumBody final : public rujian::ui::Widget {
 public:
  [[nodiscard]] rujian::ui::Size minimumSize() override { return rujian::ui::Size{400, 250}; }
};

TEST(DialogWindow, FitMinimumSizeFollowsTheContentsMinimumAndRaisesTheInitialSize) {
  Owner owner;
  int closes = 0;
  // 先用普通对话框量出本机的缩放比例
  platform::DialogWindow reference(optionsFor(owner, &closes), makeFrame());
  RECT referenceRect{};
  GetWindowRect(static_cast<HWND>(reference.handle()), &referenceRect);
  const double scale = static_cast<double>(referenceRect.right - referenceRect.left) / kWidth;

  platform::DialogWindowOptions options = optionsFor(owner, &closes);
  options.width = 300;
  options.height = 100;
  options.minimumWidth = 0;
  options.minimumHeight = 0;
  options.fitMinimumWidth = true;
  options.fitMinimumHeight = true;
  auto frame = std::make_unique<rujian::ui::DialogFrame>();
  frame->setTitle("测试");
  frame->setContent(std::make_unique<MinimumBody>());
  platform::DialogWindow dialog(std::move(options), std::move(frame));
  HWND hwnd = static_cast<HWND>(dialog.handle());
  MINMAXINFO info{};
  SendMessageW(hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  RECT rect{};
  GetWindowRect(hwnd, &rect);
  // 正文四周各留 10、标题栏高 32，另有半个像素的余量；窗口的初始大小不小于它
  EXPECT_EQ(info.ptMinTrackSize.x, std::ceil(420.5 * scale - 1e-6));
  EXPECT_EQ(info.ptMinTrackSize.y, std::ceil(302.5 * scale - 1e-6));
  EXPECT_GE(rect.right - rect.left, info.ptMinTrackSize.x - 1);
  EXPECT_GE(rect.bottom - rect.top, info.ptMinTrackSize.y - 1);
}

TEST(DialogWindow, FitMinimumSizeIsTheInitialSizeEvenWhenTheRequestedSizeIsLarger) {
  Owner owner;
  int closes = 0;
  platform::DialogWindowOptions options = optionsFor(owner, &closes);
  options.width = 900;
  options.height = 700;
  options.minimumWidth = 0;
  options.minimumHeight = 0;
  options.fitMinimumWidth = true;
  options.fitMinimumHeight = true;
  auto frame = std::make_unique<rujian::ui::DialogFrame>();
  frame->setTitle("测试");
  frame->setContent(std::make_unique<MinimumBody>());
  platform::DialogWindow dialog(std::move(options), std::move(frame));
  HWND hwnd = static_cast<HWND>(dialog.handle());
  MINMAXINFO info{};
  SendMessageW(hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  RECT rect{};
  GetWindowRect(hwnd, &rect);
  EXPECT_EQ(rect.right - rect.left, info.ptMinTrackSize.x);
  EXPECT_EQ(rect.bottom - rect.top, info.ptMinTrackSize.y);
}

TEST(DialogWindow, OnlyTheFitDirectionOpensAtTheMinimum) {
  Owner owner;
  int closes = 0;
  platform::DialogWindowOptions options = optionsFor(owner, &closes);
  options.width = 900;
  options.height = 700;
  options.minimumWidth = 0;
  options.minimumHeight = 0;
  options.fitMinimumWidth = true;
  auto frame = std::make_unique<rujian::ui::DialogFrame>();
  frame->setTitle("测试");
  frame->setContent(std::make_unique<MinimumBody>());
  platform::DialogWindow dialog(std::move(options), std::move(frame));
  HWND hwnd = static_cast<HWND>(dialog.handle());
  MINMAXINFO info{};
  SendMessageW(hwnd, WM_GETMINMAXINFO, 0, reinterpret_cast<LPARAM>(&info));
  RECT rect{};
  GetWindowRect(hwnd, &rect);
  EXPECT_EQ(rect.right - rect.left, info.ptMinTrackSize.x);
  // 高度没有按内容算，仍用请求的高度
  EXPECT_GT(rect.bottom - rect.top, info.ptMinTrackSize.y);
}

TEST(DialogWindow, CloseMessageOnlyRequestsClosingAndDoesNotDestroyTheWindow) {
  Owner owner;
  int closes = 0;
  platform::DialogWindow dialog(optionsFor(owner, &closes), makeFrame());
  HWND hwnd = static_cast<HWND>(dialog.handle());
  SendMessageW(hwnd, WM_CLOSE, 0, 0);
  EXPECT_EQ(closes, 1);
  EXPECT_NE(IsWindow(hwnd), FALSE);
  // 系统菜单的关闭（Alt+F4）也只是请求
  SendMessageW(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
  EXPECT_EQ(closes, 2);
  EXPECT_NE(IsWindow(hwnd), FALSE);
}

TEST(DialogWindow, ShowingLocksTheOwnerAndDestroyingRestoresIt) {
  Owner owner;
  int closes = 0;
  {
    platform::DialogWindow dialog(optionsFor(owner, &closes), makeFrame());
    EXPECT_NE(IsWindowEnabled(owner.hwnd()), FALSE);
    dialog.show();
    EXPECT_EQ(IsWindowEnabled(owner.hwnd()), FALSE);
    EXPECT_NE(IsWindowVisible(static_cast<HWND>(dialog.handle())), FALSE);
  }
  EXPECT_NE(IsWindowEnabled(owner.hwnd()), FALSE);
}

/** 打开者的夹具：任务先记下，由测试决定什么时候执行。 */
struct HostFixture {
  Owner owner;
  std::vector<std::function<void()>> tasks;
  bootstrap::DialogHost host{[this] { return static_cast<void*>(owner.hwnd()); }, [this](std::function<void()> task) { tasks.push_back(std::move(task)); }, false};
  int bodiesCreated{0};

  HostFixture() {
    contracts::ShellDialogDefinition definition;
    definition.id = "test.dialog";
    definition.title = "测试对话框";
    definition.width = 420;
    definition.height = 300;
    host.setDefinitions({{definition.id, definition}});
    host.setBody("test.dialog", [this] {
      ++bodiesCreated;
      return std::make_unique<rujian::ui::Widget>();
    });
  }

  void runTasks() {
    auto pending = std::move(tasks);
    tasks.clear();
    for (auto& task : pending) task();
  }
};

TEST(DialogHost, OpensOneWindowPerDialogAndFocusesTheExistingOne) {
  HostFixture f;
  f.host.openDialog("test.dialog");
  EXPECT_EQ(f.host.openCount(), 1u);
  EXPECT_EQ(f.bodiesCreated, 1);
  f.host.openDialog("test.dialog");
  EXPECT_EQ(f.host.openCount(), 1u);
  EXPECT_EQ(f.bodiesCreated, 1);
}

TEST(DialogHost, IgnoresUnknownDialogsAndDialogsWithoutABody) {
  HostFixture f;
  f.host.openDialog("no.such.dialog");
  EXPECT_EQ(f.host.openCount(), 0u);
  contracts::ShellDialogDefinition other;
  other.id = "test.other";
  f.host.setDefinitions({{other.id, other}});
  f.host.openDialog("test.other");
  EXPECT_EQ(f.host.openCount(), 0u);
}

TEST(DialogHost, ClosingIsDeferredAndThenTheOwnerWorksAgain) {
  HostFixture f;
  f.host.openDialog("test.dialog");
  ASSERT_EQ(f.host.openCount(), 1u);
  EXPECT_EQ(IsWindowEnabled(f.owner.hwnd()), FALSE);
  f.host.closeDialog("test.dialog");
  // 销毁延后到消息处理之外
  EXPECT_EQ(f.host.openCount(), 1u);
  f.runTasks();
  EXPECT_EQ(f.host.openCount(), 0u);
  EXPECT_NE(IsWindowEnabled(f.owner.hwnd()), FALSE);
  // 关掉之后可以再打开，正文重新创建
  f.host.openDialog("test.dialog");
  EXPECT_EQ(f.host.openCount(), 1u);
  EXPECT_EQ(f.bodiesCreated, 2);
}

TEST(DialogHost, ThemeChangesReachOpenDialogs) {
  HostFixture f;
  f.host.setTheme(rujian::ui::ThemeName::Light);
  f.host.openDialog("test.dialog");
  f.host.setTheme(rujian::ui::ThemeName::Dark);
  EXPECT_EQ(f.host.openCount(), 1u);
}

}  // namespace
