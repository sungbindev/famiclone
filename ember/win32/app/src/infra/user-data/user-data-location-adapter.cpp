/*
 * ------------------------------------------------------------------------
 * 名称：user-data-location-adapter.cpp
 * 说明：用户数据位置的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/user-data/user-data-location-adapter.hpp"

#include "platform/process/process-control.hpp"
#include "platform/text/wide-text.hpp"

namespace ember::win32::infra {

UserDataLocationAdapter::UserDataLocationAdapter(platform::UserDataResolution resolution, bool overridden, std::filesystem::path executable, std::filesystem::path profileDirectory)
    : resolution_(std::move(resolution)), overridden_(overridden), executable_(std::move(executable)), profileDirectory_(std::move(profileDirectory)) {}

contracts::UserDataState UserDataLocationAdapter::state() const { return contracts::UserDataState{platform::pathToUtf8(resolution_.directory), resolution_.place, overridden_}; }

std::string UserDataLocationAdapter::directoryOf(shared::UserDataPlace place) const {
  return platform::pathToUtf8(place == shared::UserDataPlace::ExecutableDirectory ? executable_.parent_path() / platform::kPortableDataDirectoryName : profileDirectory_);
}

std::string UserDataLocationAdapter::unavailableReason(shared::UserDataPlace target) const {
  return target == shared::UserDataPlace::ExecutableDirectory ? platform::portableDataUnavailableReason(executable_.parent_path()) : std::string{};
}

std::string UserDataLocationAdapter::startMove(shared::UserDataPlace target) {
  const std::vector<std::string> arguments{"--move-user-data=" + std::string(shared::userDataPlaceName(target)), "--wait-pid=" + std::to_string(platform::currentProcessId())};
  return platform::startProcess(executable_, arguments) ? std::string{} : "无法启动新的进程来移动用户数据。";
}

}  // namespace ember::win32::infra
