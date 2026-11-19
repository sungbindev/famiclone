/*
 * ------------------------------------------------------------------------
 * 名称：mapper-handlers.hpp
 * 说明：mappers 模块里外壳自己处理的请求：openPackage（选择 .emap 插件包并交给核心的 mappers.install）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用户取消选择时结果的 canceled 为 true，不是错误。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/shell/file-dialog.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** mappers 请求处理所需的依赖。 */
struct MapperDependencies {
  contracts::CoreGateway& core;
  contracts::FileDialog& dialog;
};

/** 创建 mappers 模块的处理函数。 */
[[nodiscard]] MainRequestHandlers createMapperHandlers(MapperDependencies dependencies);

}  // namespace ember::win32::entry
