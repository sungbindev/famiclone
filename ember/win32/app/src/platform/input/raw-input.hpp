/*
 * ------------------------------------------------------------------------
 * 名称：raw-input.hpp
 * 说明：原始输入采集：键盘采集（可选独占）与鼠标捕获。键盘采集时向系统登记原始输入的键盘设备（目标是主窗口），按键以“扫描码加标志”交给回调；独占时加 RIDEV_NOLEGACY，进程内不再产生传统键盘消息；
 *       鼠标捕获时登记原始输入的鼠标设备，指针隐藏并限制在一个点上，只给相对位移、按键与滚轮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：做法与 core/src/binding/keyboard-capture-win.cpp 相同，但不经 Node，直接在窗口过程里处理 WM_INPUT（作为窗口宿主的消息钩子之一调用 handle）；不用全局键盘钩子，也不用 RIDEV_INPUTSINK（窗口不在前台时收不到按键）。
 *       Win 键、Ctrl+Alt+Del 与 Alt+Tab 仍由系统处理，那时窗口会失去焦点，由使用方关闭独占；必须在窗口所在的线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>

namespace ember::win32::platform {

/** 独占键盘期间的一次按键：扫描码与前缀标志。 */
struct RawKeyEvent {
  std::uint16_t make{0};
  bool extended{false};
  bool prefixE1{false};
  bool up{false};
};

/** 鼠标按键位：与核心的鼠标消息一致，1 左键、2 右键、4 中键。 */
inline constexpr int kRawMouseLeft = 1;
inline constexpr int kRawMouseRight = 2;
inline constexpr int kRawMouseMiddle = 4;

/** 捕获鼠标期间的一次事件。 */
struct RawMouseEvent {
  /** 相对位移，设备像素。 */
  int dx{0};
  int dy{0};
  /** 事件之后按住的按键位。 */
  int buttons{0};
  /** 这次事件里刚按下与刚松开的按键位。 */
  int pressed{0};
  int released{0};
  /** 滚轮的滚动量，一格为 1，向上为正；没有滚动为 0。 */
  double wheel{0};
};

/** 原始输入采集。 */
class RawInputCapture {
 public:
  /**
   * @param window 接收 WM_INPUT 的窗口（HWND）。
   * @param onKey 独占键盘期间每次按键调用。
   * @param onMouse 捕获鼠标期间每次事件调用。
   */
  RawInputCapture(void* window, std::function<void(const RawKeyEvent&)> onKey, std::function<void(const RawMouseEvent&)> onMouse);
  ~RawInputCapture();
  RawInputCapture(const RawInputCapture&) = delete;
  RawInputCapture& operator=(const RawInputCapture&) = delete;

  /**
   * 开启或关闭键盘采集：开启后每次按键都经回调交出；exclusive 为真时同时不再产生传统键盘消息（独占），否则传统键盘消息照常产生（外壳的快捷键与菜单助记符照常工作）。
   * 返回之后是否开启（登记失败时保持原状，调用方记日志）。
   */
  bool setKeyboardCapture(bool enabled, bool exclusive);
  [[nodiscard]] bool keyboardCaptured() const { return keyboard_; }
  [[nodiscard]] bool keyboardExclusive() const { return keyboardExclusive_; }

  /**
   * 开启或关闭鼠标捕获：开启时隐藏指针并把它限制在 anchorX、anchorY 这一点（屏幕坐标）上；关闭时恢复。返回之后是否生效。
   */
  bool setMouseCapture(bool enabled, int anchorX, int anchorY);
  [[nodiscard]] bool mouseCaptured() const { return mouse_; }

  /** 处理窗口消息，作为窗口宿主的消息钩子之一使用；只处理 WM_INPUT。 */
  bool handle(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result);

 private:
  void* window_;
  std::function<void(const RawKeyEvent&)> onKey_;
  std::function<void(const RawMouseEvent&)> onMouse_;
  bool keyboard_{false};
  bool keyboardExclusive_{false};
  bool mouse_{false};
  int buttons_{0};
};

}  // namespace ember::win32::platform
