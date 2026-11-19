/*
 * ------------------------------------------------------------------------
 * 名称：raw-input.cpp
 * 说明：原始输入采集的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：键盘溢出时系统给的占位扫描码（0xFF）与扫描码 0 不是按键，忽略。
 * ------------------------------------------------------------------------
 */

#include "platform/input/raw-input.hpp"

#include <windows.h>

namespace ember::win32::platform {

namespace {

/** 键盘与鼠标设备的 HID 用法页与用法。 */
constexpr USHORT kUsagePageGeneric = 0x01;
constexpr USHORT kUsageMouse = 0x02;
constexpr USHORT kUsageKeyboard = 0x06;
/** 键盘溢出时系统给的占位扫描码。 */
constexpr USHORT kOverrunMakeCode = 0xFF;

}  // namespace

RawInputCapture::RawInputCapture(void* window, std::function<void(const RawKeyEvent&)> onKey, std::function<void(const RawMouseEvent&)> onMouse)
    : window_(window), onKey_(std::move(onKey)), onMouse_(std::move(onMouse)) {}

RawInputCapture::~RawInputCapture() {
  setKeyboardCapture(false);
  setMouseCapture(false, 0, 0);
}

bool RawInputCapture::setKeyboardCapture(bool enabled) {
  if (enabled == keyboard_) return keyboard_;
  // 关闭时目标必须是空，系统才认
  const RAWINPUTDEVICE device{kUsagePageGeneric, kUsageKeyboard, enabled ? static_cast<DWORD>(RIDEV_NOLEGACY) : static_cast<DWORD>(RIDEV_REMOVE), enabled ? static_cast<HWND>(window_) : nullptr};
  if (RegisterRawInputDevices(&device, 1, sizeof(device)) == FALSE) return keyboard_;
  keyboard_ = enabled;
  return keyboard_;
}

bool RawInputCapture::setMouseCapture(bool enabled, int anchorX, int anchorY) {
  if (enabled == mouse_) return mouse_;
  const RAWINPUTDEVICE device{kUsagePageGeneric, kUsageMouse, enabled ? static_cast<DWORD>(RIDEV_NOLEGACY) : static_cast<DWORD>(RIDEV_REMOVE), enabled ? static_cast<HWND>(window_) : nullptr};
  if (RegisterRawInputDevices(&device, 1, sizeof(device)) == FALSE) return mouse_;
  mouse_ = enabled;
  buttons_ = 0;
  if (enabled) {
    // 指针隐藏并限制在一个点上：原始输入照常给相对位移，指针不会跑到别处
    const RECT anchor{anchorX, anchorY, anchorX + 1, anchorY + 1};
    SetCursorPos(anchorX, anchorY);
    ClipCursor(&anchor);
    ShowCursor(FALSE);
  } else {
    ClipCursor(nullptr);
    ShowCursor(TRUE);
  }
  return mouse_;
}

bool RawInputCapture::handle(unsigned message, std::uintptr_t /*wParam*/, std::intptr_t lParam, std::intptr_t& /*result*/) {
  if (message != WM_INPUT) return false;
  RAWINPUT input{};
  UINT size = sizeof(input);
  const UINT read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
  if (read == static_cast<UINT>(-1)) return false;
  if (input.header.dwType == RIM_TYPEKEYBOARD) {
    const RAWKEYBOARD& key = input.data.keyboard;
    if (!keyboard_ || key.MakeCode == 0 || key.MakeCode == kOverrunMakeCode || !onKey_) return false;
    onKey_(RawKeyEvent{key.MakeCode, (key.Flags & RI_KEY_E0) != 0, (key.Flags & RI_KEY_E1) != 0, (key.Flags & RI_KEY_BREAK) != 0});
  } else if (input.header.dwType == RIM_TYPEMOUSE) {
    const RAWMOUSE& mouse = input.data.mouse;
    if (!mouse_ || !onMouse_) return false;
    RawMouseEvent event;
    if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
      event.dx = mouse.lLastX;
      event.dy = mouse.lLastY;
    }
    struct ButtonBits {
      USHORT down;
      USHORT up;
      int bit;
    };
    static constexpr ButtonBits kButtons[] = {{RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, kRawMouseLeft},
                                              {RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, kRawMouseRight},
                                              {RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, kRawMouseMiddle}};
    for (const auto& button : kButtons) {
      if ((mouse.usButtonFlags & button.down) != 0) {
        buttons_ |= button.bit;
        event.pressed |= button.bit;
      }
      if ((mouse.usButtonFlags & button.up) != 0) {
        buttons_ &= ~button.bit;
        event.released |= button.bit;
      }
    }
    if ((mouse.usButtonFlags & RI_MOUSE_WHEEL) != 0) event.wheel = static_cast<short>(mouse.usButtonData) / static_cast<double>(WHEEL_DELTA);
    event.buttons = buttons_;
    onMouse_(event);
  }
  // 返回 false：窗口宿主继续交给系统的默认处理，系统借此做 WM_INPUT 的清理
  return false;
}

}  // namespace ember::win32::platform
