/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-loading.hpp
 * 说明：加载卡带并记入最近打开：先把路径记入清单，再请核心 cartridges.load，加载失败时把它移出清单。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-26
 * 备注：打开卡带的各处（卡带模块的请求、命令行启动、文件管理里运行卡带）共用；cartridges.load 标 mainOnly，所以经网关以外壳的身份发起。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "contracts/cartridges/recent-cartridges-service.hpp"
#include "contracts/core-gateway/core-gateway.hpp"

namespace ember::win32::entry {

/**
 * 加载卡带并记入最近打开。
 * @param path 卡带路径。
 * @param done 核心的回复（成功时是 cartridges.load 的结果）。
 */
void loadAndRememberCartridge(contracts::CoreGateway& core, contracts::RecentCartridgesService& recent, const std::string& path, contracts::CoreReplyHandler done);

}  // namespace ember::win32::entry
