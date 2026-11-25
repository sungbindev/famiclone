/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-handlers.hpp
 * 说明：cartridges 模块里外壳自己处理的请求：open（选择或拖入的 .nes 文件）、recent（最近打开的清单）、openRecent（重新打开清单里的卡带），以及加载卡带并记入最近打开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：路径只来自用户在系统窗口里的选择、拖入窗口的文件、最近打开清单或启动参数，界面不能指定；
 *       cartridges.load 标 mainOnly，所以经网关以外壳的身份发起；先把路径记入清单再加载，加载失败时移出清单。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "contracts/cartridges/recent-cartridges-service.hpp"
#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/dropped-files.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "entry/main-requests/cartridge-loading.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** cartridges 请求处理所需的依赖。 */
struct CartridgeDependencies {
  contracts::CoreGateway& core;
  contracts::FileDialog& dialog;
  contracts::DroppedFilesService& dropped;
  contracts::RecentCartridgesService& recent;
};

/** 创建 cartridges 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createCartridgeHandlers(CartridgeDependencies dependencies);

}  // namespace ember::win32::entry
