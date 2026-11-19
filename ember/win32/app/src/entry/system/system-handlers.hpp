/*
 * ------------------------------------------------------------------------
 * 名称：system-handlers.hpp
 * 说明：system 模块里外壳自己处理的请求：openLogDirectory（打开日志目录）、openHelp（打开帮助文档）、updateStatus（读取升级状态）、checkUpdate（开始检查更新）、installUpdate（重启并使用新版本）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 entry/ipc/system；目录与帮助文档的位置由外壳决定，界面不能指定；installUpdate 没有下载好的新版本时回复 system.updateNotReady。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/shell/openable-directory.hpp"
#include "contracts/system/app-updater.hpp"
#include "contracts/system/help-viewer.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** system 请求处理所需的依赖。 */
struct SystemDependencies {
  contracts::OpenableDirectory& logDirectory;
  contracts::HelpViewer& help;
  contracts::AppUpdater& updater;
};

/** 创建 system 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createSystemHandlers(SystemDependencies dependencies);

}  // namespace ember::win32::entry
