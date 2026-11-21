/*
 * ------------------------------------------------------------------------
 * 名称：single-instance.cpp
 * 说明：单实例的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：与 Chromium 的 chrome/browser/process_singleton_win.cc 的做法一致；锁文件被占用时最多等 5 秒（另一个进程正在退出时锁文件会短暂存在）。
 * ------------------------------------------------------------------------
 */

#include "platform/process/single-instance.hpp"

#include <windows.h>

#include <string>
#include <system_error>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

constexpr wchar_t kWindowClass[] = L"Chrome_MessageWindow";
constexpr wchar_t kStartupMutex[] = L"Local\\ChromeProcessSingletonStartup!";
constexpr wchar_t kLockFileName[] = L"lockfile";
/** 通知已有实例的最长等待时间（毫秒）。 */
constexpr UINT kNotifyTimeoutMilliseconds = 20000;
/** 等锁文件释放的最长时间与轮询间隔（毫秒）。 */
constexpr DWORD kLockTimeoutMilliseconds = 5000;
constexpr DWORD kLockPollMilliseconds = 100;

/** 窗口标题：用户数据目录的路径。 */
std::wstring windowTitle(const std::filesystem::path& directory) { return directory.native(); }

/** 找运行着的实例的窗口。 */
HWND findRunningWindow(const std::filesystem::path& directory) { return FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, windowTitle(directory).c_str()); }

/** 当前目录与命令行：START 命令的内容。 */
std::wstring startCommand() {
  std::wstring directory(MAX_PATH, L'\0');
  directory.resize(GetCurrentDirectoryW(static_cast<DWORD>(directory.size()), directory.data()));
  std::wstring command(L"START\0", 6);
  command += directory;
  command.push_back(L'\0');
  command += GetCommandLineW();
  command.push_back(L'\0');
  return command;
}

/** 向已有实例发送 START 命令；对方回复 TRUE 才算通知到。 */
bool notify(HWND remote) {
  // 本进程刚被用户操作拉起（如双击文件），有前台权限；把它交给对方，对方才能把窗口提到最前
  DWORD remoteProcess = 0;
  GetWindowThreadProcessId(remote, &remoteProcess);
  if (remoteProcess != 0) AllowSetForegroundWindow(remoteProcess);
  const std::wstring command = startCommand();
  COPYDATASTRUCT data{};
  data.dwData = 0;
  data.cbData = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
  data.lpData = const_cast<wchar_t*>(command.c_str());
  DWORD_PTR result = 0;
  const LRESULT sent = SendMessageTimeoutW(remote, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data), SMTO_ABORTIFHUNG, kNotifyTimeoutMilliseconds, &result);
  return sent != 0 && result == TRUE;
}

/** 独占创建锁文件；被别的进程占用时每 100 毫秒重试一次，最多等 5 秒。 */
HANDLE createLockFile(const std::filesystem::path& path) {
  const DWORD start = GetTickCount();
  for (;;) {
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (handle != INVALID_HANDLE_VALUE) return handle;
    if (GetLastError() != ERROR_SHARING_VIOLATION || GetTickCount() - start >= kLockTimeoutMilliseconds) return INVALID_HANDLE_VALUE;
    Sleep(kLockPollMilliseconds);
  }
}

}  // namespace

struct SingleInstance::Impl {
  static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<SingleInstance*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_COPYDATA && self != nullptr) {
      const auto* data = reinterpret_cast<const COPYDATASTRUCT*>(lParam);
      // 命令格式：START 加三段以空字符结尾的文字；格式不对的也回复 TRUE（同 Chromium），只是不提前窗口
      static constexpr std::size_t kMinimumSize = 7 * sizeof(wchar_t);
      if (data != nullptr && data->cbData >= kMinimumSize && data->cbData % sizeof(wchar_t) == 0) {
        const std::wstring text(static_cast<const wchar_t*>(data->lpData), data->cbData / sizeof(wchar_t));
        if (text.compare(0, 5, L"START") == 0 && text.size() > 5 && text[5] == L'\0' && self->onSecondInstance_) {
          // 命令的三段：START、后启动进程的当前目录、它的命令行；缺段时按空处理
          std::wstring directory;
          std::wstring commandLine;
          const std::size_t directoryEnd = text.find(L'\0', 6);
          if (directoryEnd != std::wstring::npos) {
            directory = text.substr(6, directoryEnd - 6);
            const std::size_t commandEnd = text.find(L'\0', directoryEnd + 1);
            if (commandEnd != std::wstring::npos) commandLine = text.substr(directoryEnd + 1, commandEnd - directoryEnd - 1);
          }
          self->onSecondInstance_(directory, commandLine);
        }
      }
      return TRUE;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }

  /** 创建接收通知的消息窗口。 */
  static HWND createWindow(SingleInstance& owner) {
    static const ATOM registered = [] {
      WNDCLASSEXW windowClass{sizeof(windowClass)};
      windowClass.lpfnWndProc = &Impl::procedure;
      windowClass.hInstance = GetModuleHandleW(nullptr);
      windowClass.lpszClassName = kWindowClass;
      return RegisterClassExW(&windowClass);
    }();
    if (registered == 0) return nullptr;
    HWND hwnd = CreateWindowExW(0, kWindowClass, windowTitle(owner.userDataDirectory_).c_str(), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (hwnd != nullptr) SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&owner));
    return hwnd;
  }
};

SingleInstance::SingleInstance(std::filesystem::path userDataDirectory, std::function<void()> onSecondInstance)
    : SingleInstance(std::move(userDataDirectory), SecondInstanceHandler([onSecondInstance = std::move(onSecondInstance)](const std::wstring&, const std::wstring&) {
        if (onSecondInstance) onSecondInstance();
      })) {}

SingleInstance::SingleInstance(std::filesystem::path userDataDirectory, SecondInstanceHandler onSecondInstance)
    : userDataDirectory_(std::move(userDataDirectory)), onSecondInstance_(std::move(onSecondInstance)) {}

SingleInstance::~SingleInstance() {
  if (window_ != nullptr) {
    SetWindowLongPtrW(static_cast<HWND>(window_), GWLP_USERDATA, 0);
    DestroyWindow(static_cast<HWND>(window_));
  }
  if (lockFile_ != nullptr) CloseHandle(lockFile_);
}

SingleInstanceResult SingleInstance::acquire() {
  HWND remote = findRunningWindow(userDataDirectory_);
  if (remote == nullptr) {
    // 创建过程由全局互斥量保护：同一时刻只有一个进程在创建窗口
    HANDLE mutex = CreateMutexW(nullptr, FALSE, kStartupMutex);
    if (mutex == nullptr) return SingleInstanceResult::InUse;
    WaitForSingleObject(mutex, INFINITE);
    remote = findRunningWindow(userDataDirectory_);
    if (remote == nullptr) {
      std::error_code error;
      std::filesystem::create_directories(userDataDirectory_, error);
      HANDLE lock = createLockFile(userDataDirectory_ / kLockFileName);
      if (lock != INVALID_HANDLE_VALUE) {
        HWND window = Impl::createWindow(*this);
        if (window == nullptr) {
          CloseHandle(lock);
        } else {
          lockFile_ = lock;
          window_ = window;
        }
      }
    }
    ReleaseMutex(mutex);
    CloseHandle(mutex);
  }
  if (window_ != nullptr) return SingleInstanceResult::Acquired;
  if (remote == nullptr) remote = findRunningWindow(userDataDirectory_);
  return remote != nullptr && notify(remote) ? SingleInstanceResult::NotifiedExisting : SingleInstanceResult::InUse;
}

}  // namespace ember::win32::platform
