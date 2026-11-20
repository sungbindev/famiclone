/*
 * ------------------------------------------------------------------------
 * 名称：display-test.cpp
 * 说明：运行画面子窗口与输入来源的冒烟测试：能在一个隐藏的父窗口里创建画面子窗口、设置位置、上传帧与提示位图、挖洞并显示而不出错（没有硬件 Direct3D 11 设备的电脑上跳过）；
 *       XInput 手柄读取在没有手柄时给出空清单；原始输入采集在共享、独占与关闭之间切换键盘采集不出错。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：画面的像素内容由原生冒烟脚本截图核对，这里只检查调用不失败。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <windows.h>

#include <vector>

#include "platform/display/screen-window.hpp"
#include "platform/input/raw-input.hpp"
#include "platform/input/xinput-gamepads.hpp"

namespace ember::win32 {
namespace {

/** 隐藏的父窗口。 */
class HiddenParent {
 public:
  HiddenParent() {
    static const ATOM registered = [] {
      WNDCLASSEXW windowClass{sizeof(windowClass)};
      windowClass.lpfnWndProc = DefWindowProcW;
      windowClass.hInstance = GetModuleHandleW(nullptr);
      windowClass.lpszClassName = L"EmberDisplayTestParent";
      return RegisterClassExW(&windowClass);
    }();
    EXPECT_NE(registered, 0);
    window_ = CreateWindowExW(0, L"EmberDisplayTestParent", L"", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, 0, 640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  }
  ~HiddenParent() {
    if (window_ != nullptr) DestroyWindow(window_);
  }
  [[nodiscard]] HWND handle() const { return window_; }

 private:
  HWND window_{nullptr};
};

TEST(ScreenWindow, CreatesShowsFramesAndOverlaysWithoutErrors) {
  HiddenParent parent;
  ASSERT_NE(parent.handle(), nullptr);
  std::unique_ptr<platform::ScreenWindow> screen;
  try {
    screen = std::make_unique<platform::ScreenWindow>(parent.handle());
  } catch (const std::runtime_error& error) {
    GTEST_SKIP() << "没有硬件的 Direct3D 11 设备：" << error.what();
  }
  EXPECT_NE(screen->handle(), nullptr);
  screen->setBounds(shared::ScreenRect{10, 20, 320, 300});
  EXPECT_EQ(screen->bounds(), (shared::ScreenRect{10, 20, 320, 300}));
  std::vector<std::uint32_t> frame(256 * 240, 0xff0000ffU);
  screen->setFrame(frame.data(), 256, 240);
  screen->setFrameVisible(true);
  screen->setView(shared::ScreenAspect::Ratio4x3, shared::ScreenScaling::Integer, 8);
  screen->render();
  // 帧的大小变化（学习机的画面比 FC 大）
  std::vector<std::uint32_t> large(640 * 480, 0xff00ff00U);
  screen->setFrame(large.data(), 640, 480);
  screen->render();
  std::vector<std::uint8_t> overlay(320 * 300 * 4, 0x80);
  screen->setOverlay(overlay.data(), 320, 300, shared::ScreenRect{100, 200, 200, 80});
  screen->render();
  // 提示框超出画面、与画面不相交时不出错
  screen->setOverlay(overlay.data(), 320, 300, shared::ScreenRect{300, 280, 100, 100});
  screen->render();
  screen->setOverlay(overlay.data(), 320, 300, shared::ScreenRect{0, 0, 5, 5});
  screen->render();
  screen->setOverlay(nullptr, 0, 0, shared::ScreenRect{});
  // 弹层盖到画面上时挖洞，再还原
  screen->setHoles({shared::ScreenRect{50, 30, 100, 80}});
  screen->setHoles({});
  // 子窗口大小变化后交换链跟着变，大小为 0 时隐藏
  screen->setBounds(shared::ScreenRect{10, 20, 200, 150});
  screen->render();
  screen->setBounds(shared::ScreenRect{});
  screen->render();
}

TEST(XInputGamepads, ReadsNothingWhenNoPadIsConnectedAndNeverFails) {
  platform::XInputGamepads pads;
  for (int i = 0; i < 3; ++i) {
    const auto readings = pads.read();
    for (const auto& pad : readings) {
      EXPECT_EQ(pad.buttons.size(), 17U);
      EXPECT_EQ(pad.axes.size(), 4U);
      EXPECT_GE(pad.index, 0);
      EXPECT_LT(pad.index, 4);
    }
  }
}

TEST(RawInputCapture, KeyboardCaptureCanBeSwitchedBetweenSharedExclusiveAndOffWithoutLeavingTheKeyboardDead) {
  HiddenParent parent;
  ASSERT_NE(parent.handle(), nullptr);
  platform::RawInputCapture capture(parent.handle(), [](const platform::RawKeyEvent&) {}, [](const platform::RawMouseEvent&) {});
  EXPECT_FALSE(capture.keyboardCaptured());
  EXPECT_FALSE(capture.keyboardExclusive());
  // 共享、独占、关闭之间来回切换，状态与调用一致；重复调用无副作用
  EXPECT_TRUE(capture.setKeyboardCapture(true, false));
  EXPECT_TRUE(capture.keyboardCaptured());
  EXPECT_FALSE(capture.keyboardExclusive());
  EXPECT_TRUE(capture.setKeyboardCapture(true, true));
  EXPECT_TRUE(capture.setKeyboardCapture(true, true));
  EXPECT_TRUE(capture.keyboardExclusive());
  EXPECT_TRUE(capture.setKeyboardCapture(true, false));
  EXPECT_TRUE(capture.keyboardCaptured());
  EXPECT_FALSE(capture.keyboardExclusive());
  EXPECT_FALSE(capture.setKeyboardCapture(false, false));
  EXPECT_FALSE(capture.keyboardCaptured());
  EXPECT_FALSE(capture.keyboardExclusive());
  EXPECT_FALSE(capture.setMouseCapture(false, 0, 0));
}

}  // namespace
}  // namespace ember::win32
