/*
 * ------------------------------------------------------------------------
 * 名称：machine-handlers.hpp
 * 说明：machines 模块里外壳自己处理的请求：openBios（选择 BIOS 文件并交给核心的 machines.setBios）、openBiosDirectory（用资源管理器打开 BIOS 目录）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 entry/ipc/machines；用户取消选择时结果的 canceled 为 true，不是错误；BIOS 目录由外壳决定，界面不能指定。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "contracts/shell/openable-directory.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** machines 请求处理所需的依赖。 */
struct MachineDependencies {
  contracts::CoreGateway& core;
  contracts::FileDialog& dialog;
  contracts::OpenableDirectory& biosDirectory;
};

/** 创建 machines 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createMachineHandlers(MachineDependencies dependencies);

}  // namespace ember::win32::entry
