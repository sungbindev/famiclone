/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-handlers.hpp
 * 说明：diskimages 模块里外壳自己处理的请求：openRoot（把保存的镜像目录交给核心）、pickRoot（选择镜像目录）、importDropped（导入拖入窗口的文件）、loadImage（把镜像载入软驱）、revealImage（在资源管理器里显示镜像文件）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：镜像目录的路径与拖入文件的路径只在外壳里：目录由用户在系统窗口里选，保存在外壳设置（shell-state.json），经 diskimages.setRoot 交给核心；
 *       用户取消选择时结果的 canceled 为 true，不是错误。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/directory-opener.hpp"
#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "contracts/shell/shell-state-service.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** diskimages 请求处理所需的依赖。 */
struct DiskimagesDependencies {
  contracts::CoreGateway& core;
  contracts::FileDialog& dialog;
  contracts::DroppedFilesService& dropped;
  contracts::DirectoryOpener& directories;
  contracts::ShellStateService& shellState;
};

/** 创建 diskimages 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createDiskimagesHandlers(DiskimagesDependencies dependencies);

}  // namespace ember::win32::entry
