/*
 * ------------------------------------------------------------------------
 * 名称：user-data-move.cpp
 * 说明：移动用户数据的运行方式的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/user-data-move.hpp"

#include "platform/dialogs/message-box.hpp"
#include "platform/files/known-folders.hpp"
#include "platform/files/user-data-location.hpp"
#include "platform/process/process-control.hpp"

namespace ember::win32::bootstrap {

namespace {

/** 最多等旧进程退出多久（毫秒）。 */
constexpr unsigned kWaitMilliseconds = 30000;
constexpr const char* kTitle = "Ember";

}  // namespace

int runUserDataMove(const shared::LaunchArguments& launch) {
  const std::filesystem::path executable = platform::executablePath();
  if (launch.waitProcessId && !platform::waitForProcessExit(*launch.waitProcessId, kWaitMilliseconds)) {
    platform::showErrorMessage(kTitle, "无法切换用户数据目录：原来的 Ember 没有退出。用户数据保持原样。");
    return 1;
  }
  const platform::UserDataMoveResult result = platform::moveUserData(executable.parent_path(), platform::defaultUserDataDirectory(), *launch.moveUserData);
  if (!result.ok) platform::showErrorMessage(kTitle, "无法切换用户数据目录：" + result.message + "\n用户数据保持原样。");
  // 无论成败都重新启动软件，让用户回到软件里
  static_cast<void>(platform::startProcess(executable, {}));
  return result.ok ? 0 : 1;
}

}  // namespace ember::win32::bootstrap
