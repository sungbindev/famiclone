/*
 * ------------------------------------------------------------------------
 * 名称：update-apply.cpp
 * 说明：应用升级的运行方式的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/update-apply.hpp"

#include <windows.h>

#include <chrono>
#include <cwctype>
#include <thread>

#include "platform/dialogs/message-box.hpp"
#include "platform/files/known-folders.hpp"
#include "platform/process/process-control.hpp"
#include "platform/text/wide-text.hpp"
#include "shared/update/update-manifest.hpp"

namespace ember::win32::bootstrap {

namespace {

namespace fs = std::filesystem;

constexpr const char* kTitle = "Ember";
/** 最多等旧进程退出多久（毫秒）。 */
constexpr unsigned kWaitMilliseconds = 30000;
/** 覆盖旧程序的重试次数与间隔。 */
constexpr int kCopyAttempts = 150;
constexpr int kCopyIntervalMilliseconds = 200;

/** 自己的文件名是不是 Ember-<版本号>.exe。 */
bool isStagedName(const fs::path& file) {
  const std::string name = platform::pathToUtf8(file.filename());
  if (name.rfind("Ember-", 0) != 0 || name.size() <= 10 || name.compare(name.size() - 4, 4, ".exe") != 0) return false;
  return shared::parseVersion(name.substr(6, name.size() - 10)).has_value();
}

bool hasExeExtension(const fs::path& file) {
  std::wstring extension = file.extension().wstring();
  for (wchar_t& c : extension) c = static_cast<wchar_t>(::towlower(c));
  return extension == L".exe";
}

}  // namespace

int runUpdateApply(const shared::LaunchArguments& launch) {
  const fs::path self = platform::executablePath();
  const fs::path target = platform::pathFromUtf8(*launch.applyUpdateTarget);
  std::error_code error;
  if (!isStagedName(self) || !hasExeExtension(target) || !fs::is_regular_file(target, error) || fs::equivalent(self, target, error)) {
    platform::showErrorMessage(kTitle, "无法升级：这个程序不是用来直接运行的升级文件。");
    return 1;
  }
  if (launch.waitProcessId && !platform::waitForProcessExit(*launch.waitProcessId, kWaitMilliseconds)) {
    platform::showErrorMessage(kTitle, "无法升级：原来的 Ember 没有退出。");
    return 1;
  }

  DWORD lastError = ERROR_SUCCESS;
  bool copied = false;
  // 先复制到目标旁边的临时文件，再整体替换目标，避免复制到一半出错（磁盘满等）把原程序破坏
  const fs::path temporary = fs::path(target.wstring() + L".new");
  for (int attempt = 0; attempt < kCopyAttempts && !copied; ++attempt) {
    if (CopyFileW(self.c_str(), temporary.c_str(), FALSE) == FALSE) {
      lastError = GetLastError();
    } else if (MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING) == FALSE) {
      lastError = GetLastError();
      DeleteFileW(temporary.c_str());
    } else {
      copied = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(kCopyIntervalMilliseconds));
  }
  if (!copied) {
    platform::showErrorMessage(kTitle, "无法升级：不能覆盖原来的程序（错误码 " + std::to_string(lastError) + "）。新版本在 " + platform::pathToUtf8(self) + "，可以手动复制过去。原来的版本保持不变。");
  }
  // 无论成败都启动原位置的程序，让用户回到软件里
  static_cast<void>(platform::startProcess(target, {}));
  return copied ? 0 : 1;
}

}  // namespace ember::win32::bootstrap
