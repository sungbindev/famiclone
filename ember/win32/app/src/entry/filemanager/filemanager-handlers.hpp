/*
 * ------------------------------------------------------------------------
 * 名称：filemanager-handlers.hpp
 * 说明：filemanager 模块里外壳自己处理的请求：openRoot（把保存的目录交给核心）、pickRoot（选择目录）、importDropped（导入拖入窗口的文件）、loadImage（把镜像载入软驱）、revealImage（在资源管理器里显示文件）、runCartridge（运行 nes 卡带）、loadTape（把音频载入录音机）、openFile（用系统默认的程序打开文件）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：目录的路径与拖入文件的路径只在外壳里：目录由用户在系统窗口里选，保存在外壳设置（shell-state.json），经 filemanager.setRoot 交给核心；
 *       用户取消选择时结果的 canceled 为 true，不是错误。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/cartridges/recent-cartridges-service.hpp"
#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/directory-opener.hpp"
#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** filemanager 请求处理所需的依赖。 */
struct FileManagerDependencies {
  contracts::CoreGateway& core;
  contracts::FileDialog& dialog;
  contracts::DroppedFilesService& dropped;
  contracts::DirectoryOpener& directories;
  contracts::ShellStateService& shellState;
  contracts::RecentCartridgesService& recent;
};

/** 创建 filemanager 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createFileManagerHandlers(FileManagerDependencies dependencies);

}  // namespace ember::win32::entry
