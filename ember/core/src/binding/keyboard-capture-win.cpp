/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-capture-win.cpp
 * 说明：Windows 键盘独占的原生模块（ember_keyboard.node）：导出 enable、disable；开启后，应用内所有窗口收不到任何键盘消息，按键只以“扫描码+标志”的形式交给 JavaScript。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：做法：向系统登记“原始输入”键盘设备（RIDEV_NOLEGACY，目标是主窗口），再子类化主窗口接收 WM_INPUT；
 *       登记了 NOLEGACY 后，本进程不再产生 WM_KEYDOWN 等传统键盘消息，Chromium（含指针锁定时对 Esc 的处理）也就收不到按键，
 *       所以 Esc、Tab、F 键、Ctrl、Alt 组合都能交给学习机；系统自己的 Win 键、Ctrl+Alt+Del 与 Alt+Tab 仍由系统处理，那时窗口会失去焦点，由 JavaScript 一侧关闭独占。
 *       不使用全局键盘钩子，也不使用 RIDEV_INPUTSINK（窗口不在前台时收不到按键）。
 *       enable 与 disable 必须在创建主窗口的线程（Electron 的主进程 JavaScript 线程）调用；窗口过程运行在同一线程，经线程安全函数把按键交给 JavaScript。
 *       本文件不引用核心的其他代码，只使用 Windows 与 Node-API，所以放在 binding 目录；只有 Windows 构建它。
 * ------------------------------------------------------------------------
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commctrl.h>

#include <cstdint>
#include <cstring>

#include <napi.h>

namespace {

/** 主窗口子类的编号。 */
constexpr UINT_PTR SUBCLASS_ID = 1;
/** 键盘设备的 HID 用法页与用法。 */
constexpr USHORT USAGE_PAGE_GENERIC = 0x01;
constexpr USHORT USAGE_KEYBOARD = 0x06;
/** 键盘溢出时系统给的占位扫描码，不是按键。 */
constexpr USHORT OVERRUN_MAKE_CODE = 0xFF;

/** 交给 JavaScript 的一次按键。 */
struct KeyData {
  USHORT make;
  USHORT flags;
};

/** 当前开启独占的窗口；没有开启时为 nullptr。 */
HWND g_window = nullptr;
/** 把按键交给 JavaScript 的回调。 */
Napi::ThreadSafeFunction g_callback;

/** 处理一条原始输入消息，把键盘按键交给 JavaScript。 */
void HandleRawInput(LPARAM lParam) {
  RAWINPUT input{};
  UINT size = sizeof(input);
  const UINT read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
  if (read == static_cast<UINT>(-1) || input.header.dwType != RIM_TYPEKEYBOARD) return;
  const RAWKEYBOARD& key = input.data.keyboard;
  if (key.MakeCode == 0 || key.MakeCode == OVERRUN_MAKE_CODE) return;

  auto* data = new KeyData{key.MakeCode, key.Flags};
  const napi_status status = g_callback.NonBlockingCall(data, [](Napi::Env env, Napi::Function callback, KeyData* value) {
    Napi::Object event = Napi::Object::New(env);
    event.Set("make", Napi::Number::New(env, value->make));
    event.Set("e0", Napi::Boolean::New(env, (value->flags & RI_KEY_E0) != 0));
    event.Set("e1", Napi::Boolean::New(env, (value->flags & RI_KEY_E1) != 0));
    event.Set("up", Napi::Boolean::New(env, (value->flags & RI_KEY_BREAK) != 0));
    delete value;
    callback.Call({event});
  });
  if (status != napi_ok) delete data;
}

/** 主窗口的子类过程：处理 WM_INPUT，其余消息与 WM_INPUT 的清理交给原窗口过程。 */
LRESULT CALLBACK KeyboardSubclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR /*id*/, DWORD_PTR /*data*/) {
  if (message == WM_INPUT) {
    HandleRawInput(lParam);
  } else if (message == WM_NCDESTROY) {
    RemoveWindowSubclass(window, KeyboardSubclassProc, SUBCLASS_ID);
  }
  return DefSubclassProc(window, message, wParam, lParam);
}

/** 向系统取消键盘设备的登记，传统键盘消息恢复。 */
void UnregisterKeyboard() {
  RAWINPUTDEVICE device{};
  device.usUsagePage = USAGE_PAGE_GENERIC;
  device.usUsage = USAGE_KEYBOARD;
  device.dwFlags = RIDEV_REMOVE;
  device.hwndTarget = nullptr;
  RegisterRawInputDevices(&device, 1, sizeof(device));
}

/** 关闭独占：取消登记、摘掉子类、释放回调；没有开启时什么也不做。 */
void Release() {
  if (g_window == nullptr) return;
  UnregisterKeyboard();
  if (IsWindow(g_window)) RemoveWindowSubclass(g_window, KeyboardSubclassProc, SUBCLASS_ID);
  g_window = nullptr;
  g_callback.Release();
}

/**
 * enable(windowHandle: Buffer, callback: (event) => void): number
 * 开启键盘独占。windowHandle 是 BrowserWindow.getNativeWindowHandle() 的结果；返回 0 表示成功，否则为 Windows 错误码；已开启时先关闭再开启。
 */
Napi::Value Enable(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsBuffer() || !info[1].IsFunction()) {
    throw Napi::TypeError::New(env, "enable 需要窗口句柄（Buffer）与回调函数");
  }
  const auto handle = info[0].As<Napi::Buffer<std::uint8_t>>();
  if (handle.Length() < sizeof(HWND)) throw Napi::TypeError::New(env, "窗口句柄的长度不对");
  HWND window = nullptr;
  std::memcpy(&window, handle.Data(), sizeof(HWND));
  if (!IsWindow(window)) return Napi::Number::New(env, ERROR_INVALID_WINDOW_HANDLE);

  Release();

  if (!SetWindowSubclass(window, KeyboardSubclassProc, SUBCLASS_ID, 0)) return Napi::Number::New(env, GetLastError());
  RAWINPUTDEVICE device{};
  device.usUsagePage = USAGE_PAGE_GENERIC;
  device.usUsage = USAGE_KEYBOARD;
  device.dwFlags = RIDEV_NOLEGACY;
  device.hwndTarget = window;
  if (!RegisterRawInputDevices(&device, 1, sizeof(device))) {
    const DWORD error = GetLastError();
    RemoveWindowSubclass(window, KeyboardSubclassProc, SUBCLASS_ID);
    return Napi::Number::New(env, error);
  }
  g_callback = Napi::ThreadSafeFunction::New(env, info[1].As<Napi::Function>(), "ember-keyboard-capture", 0, 1);
  g_window = window;
  return Napi::Number::New(env, 0);
}

/** disable(): void：关闭键盘独占，没有开启时什么也不做。 */
Napi::Value Disable(const Napi::CallbackInfo& info) {
  Release();
  return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("enable", Napi::Function::New(env, Enable, "enable"));
  exports.Set("disable", Napi::Function::New(env, Disable, "disable"));
  return exports;
}

}  // namespace

NODE_API_MODULE(ember_keyboard, Init)
