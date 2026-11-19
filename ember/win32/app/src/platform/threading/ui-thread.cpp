/*
 * ------------------------------------------------------------------------
 * 名称：ui-thread.cpp
 * 说明：界面线程任务投递的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "platform/threading/ui-thread.hpp"

#include <windows.h>

#include <stdexcept>
#include <utility>

namespace ember::win32::platform {

namespace {

constexpr wchar_t kWindowClass[] = L"EmberUiThreadWindow";
/** 有新任务的窗口消息。 */
constexpr UINT kTasksMessage = WM_APP + 1;

}  // namespace

struct UiThread::Impl {
  static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<UiThread*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self != nullptr && message == kTasksMessage) {
      std::vector<std::function<void()>> tasks;
      {
        std::lock_guard lock(self->mutex_);
        tasks.swap(self->queue_);
      }
      for (auto& task : tasks) task();
      return 0;
    }
    if (self != nullptr && message == WM_TIMER) {
      const auto id = static_cast<unsigned>(wParam);
      KillTimer(hwnd, wParam);
      const auto found = self->delayed_.find(id);
      if (found == self->delayed_.end()) return 0;
      auto task = std::move(found->second);
      self->delayed_.erase(found);
      task();
      return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }
};

UiThread::UiThread() : threadId_(GetCurrentThreadId()) {
  static const ATOM registered = [] {
    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.lpfnWndProc = &Impl::procedure;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kWindowClass;
    return RegisterClassExW(&windowClass);
  }();
  if (registered == 0) throw std::runtime_error("注册界面线程窗口类失败");
  HWND hwnd = CreateWindowExW(0, kWindowClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (hwnd == nullptr) throw std::runtime_error("创建界面线程窗口失败");
  SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  window_ = hwnd;
}

UiThread::~UiThread() {
  auto* hwnd = static_cast<HWND>(window_);
  SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
  DestroyWindow(hwnd);
}

void UiThread::post(std::function<void()> task) {
  {
    std::lock_guard lock(mutex_);
    queue_.push_back(std::move(task));
  }
  PostMessageW(static_cast<HWND>(window_), kTasksMessage, 0, 0);
}

unsigned UiThread::postDelayed(unsigned milliseconds, std::function<void()> task) {
  const unsigned id = nextId_++;
  delayed_[id] = std::move(task);
  SetTimer(static_cast<HWND>(window_), id, milliseconds, nullptr);
  return id;
}

void UiThread::cancelDelayed(unsigned id) {
  if (delayed_.erase(id) > 0) KillTimer(static_cast<HWND>(window_), id);
}

bool UiThread::isCurrent() const { return GetCurrentThreadId() == threadId_; }

void UiThread::pumpMessages() {
  MSG message;
  while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
}

}  // namespace ember::win32::platform
