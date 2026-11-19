/*
 * ------------------------------------------------------------------------
 * 名称：file-shell-state-service.cpp
 * 说明：界面状态服务的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/shell-state/file-shell-state-service.hpp"

namespace ember::win32::infra {

FileShellStateService::FileShellStateService(platform::JsonFileStore& store) : store_(store) {
  if (const auto saved = store_.read()) state_ = shared::parseShellState(*saved);
}

void FileShellStateService::updateLayout(const shared::ShellLayout& layout) {
  shared::ShellState next = state_;
  next.layout = layout;
  replace(std::move(next));
}

void FileShellStateService::updateWindow(const shared::WindowPlacement& placement) {
  shared::ShellState next = state_;
  next.window = placement;
  replace(std::move(next));
}

void FileShellStateService::updateReleaseNotesVersion(const std::string& version) {
  shared::ShellState next = state_;
  next.releaseNotesVersion = version;
  replace(std::move(next));
}

void FileShellStateService::replace(shared::ShellState next) {
  state_ = std::move(next);
  store_.write(shared::toJson(state_));
}

}  // namespace ember::win32::infra
