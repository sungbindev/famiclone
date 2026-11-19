/*
 * ------------------------------------------------------------------------
 * 名称：release-notes-prompt.hpp
 * 说明：升级后第一次启动新版本时，自动打开帮助文档里的“更新记录”页；每个版本只提示一次。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只有支持升级的运行方式才提示，开发版与不能覆盖程序的位置不提示；界面状态里没有记录时是第一次使用，只记下当前版本、不提示；先记下版本再打开页面，页面打不开也不会每次启动都重试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "contracts/logging/error-logger.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "contracts/system/app-updater.hpp"
#include "contracts/system/help-viewer.hpp"

namespace ember::win32::logic {

/**
 * 需要时打开更新记录页，并记下已经提示过的版本。
 * @param updater 升级服务：不支持升级时什么都不做。
 * @param currentVersion 当前运行的应用版本。
 * @param state 界面状态服务：读取与保存上次提示的版本。
 * @param help 帮助文档。
 * @param logger 打开页面失败时写日志。
 */
void showReleaseNotesOnce(const contracts::AppUpdater& updater, const std::string& currentVersion, contracts::ShellStateService& state, contracts::HelpViewer& help, contracts::ErrorLogger& logger);

}  // namespace ember::win32::logic
