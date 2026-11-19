/*
 * ------------------------------------------------------------------------
 * 名称：user-data-controller.cpp
 * 说明：“应用”分组里“用户数据目录”一行的控制器的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/preferences/user-data-controller.hpp"

namespace ember::win32::logic {

namespace {

constexpr const char* kSwitchToExecutable = "改用软件所在目录";
constexpr const char* kSwitchToProfile = "改用用户数据目录";
constexpr const char* kOverriddenReason = "本次启动用 --user-data-dir 指定了数据目录，不能切换。";
constexpr const char* kConfirmTitle = "切换用户数据目录";

}  // namespace

UserDataController::UserDataController(contracts::UserDataLocation& location, contracts::OpenableDirectory& directory, contracts::Confirmation& confirmation)
    : location_(location), directory_(directory), confirmation_(confirmation) {}

std::function<void()> UserDataController::attach(contracts::ShellApi& shell) {
  shell_ = &shell;
  return [this] { shell_ = nullptr; };
}

shared::UserDataPlace UserDataController::targetPlace() const {
  return location_.state().place == shared::UserDataPlace::ExecutableDirectory ? shared::UserDataPlace::UserProfile : shared::UserDataPlace::ExecutableDirectory;
}

contracts::UserDataView UserDataController::view() const {
  const contracts::UserDataState state = location_.state();
  contracts::UserDataView result;
  result.directory = state.directory;
  const shared::UserDataPlace target = targetPlace();
  result.switchLabel = target == shared::UserDataPlace::ExecutableDirectory ? kSwitchToExecutable : kSwitchToProfile;
  result.unavailableReason = state.overridden ? std::string(kOverriddenReason) : location_.unavailableReason(target);
  return result;
}

void UserDataController::openDirectory() {
  if (shell_ == nullptr) return;
  const std::string error = directory_.open();
  if (!error.empty()) shell_->notify(error, contracts::NotificationLevel::Error);
}

void UserDataController::switchPlace() {
  if (shell_ == nullptr) return;
  const contracts::UserDataView current = view();
  if (!current.unavailableReason.empty()) {
    shell_->notify(current.unavailableReason, contracts::NotificationLevel::Error);
    return;
  }
  const shared::UserDataPlace target = targetPlace();
  const std::string text = "软件会先关闭，把用户数据复制到“" + location_.directoryOf(target) + "”，核对无误后自动重新启动；复制成功后会删除原位置的数据。\n\n继续吗？";
  if (!confirmation_.ask(kConfirmTitle, text)) return;
  const std::string error = location_.startMove(target);
  if (!error.empty()) {
    shell_->notify(error, contracts::NotificationLevel::Error);
    return;
  }
  shell_->quit();
}

}  // namespace ember::win32::logic
