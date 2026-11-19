/*
 * ------------------------------------------------------------------------
 * 名称：mapper-handlers.cpp
 * 说明：mappers 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/mappers/mapper-handlers.hpp"

#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

MainRequestHandlers createMapperHandlers(MapperDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["mappers.openPackage"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto path = dependencies.dialog.pickFile({"安装 mapper 插件", {{"mapper 插件包", {"emap"}}, {"所有文件", {"*"}}}, "mapperPackage"});
    if (!path) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    dependencies.core.request("mappers.install", nlohmann::json{{"path", *path}}, [done](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"canceled", false}, {"plugin", reply.result.value("plugin", nlohmann::json(nullptr))}}) : std::move(reply));
    });
  };

  return handlers;
}

}  // namespace ember::win32::entry
