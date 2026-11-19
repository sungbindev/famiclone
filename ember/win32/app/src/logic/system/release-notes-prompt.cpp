/*
 * ------------------------------------------------------------------------
 * 名称：release-notes-prompt.cpp
 * 说明：升级后第一次启动新版本时打开更新记录页的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/system/release-notes-prompt.hpp"

namespace ember::win32::logic {

void showReleaseNotesOnce(const contracts::AppUpdater& updater, const std::string& currentVersion, contracts::ShellStateService& state, contracts::HelpViewer& help, contracts::ErrorLogger& logger) {
  if (updater.status().state == "unsupported") return;
  const std::string recorded = state.get().releaseNotesVersion.value_or(std::string());
  if (recorded == currentVersion) return;
  state.updateReleaseNotesVersion(currentVersion);
  // 没有记录：第一次使用，不提示
  if (recorded.empty()) return;
  const std::string error = help.open(std::string("changelog"));
  if (!error.empty()) logger.write({std::nullopt, "shell.mainError", "升级后打开更新记录失败：" + error, nlohmann::json{{"source", "releaseNotes"}}});
}

}  // namespace ember::win32::logic
