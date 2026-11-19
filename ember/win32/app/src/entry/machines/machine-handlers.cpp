/*
 * ------------------------------------------------------------------------
 * 名称：machine-handlers.cpp
 * 说明：machines 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/machines/machine-handlers.hpp"

#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

MainRequestHandlers createMachineHandlers(MachineDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["machines.openBios"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"machine"});
    const std::string machine = reader.requiredString("machine");
    const auto path = dependencies.dialog.pickFile({"添加 BIOS", {{"BIOS 文件", {"bin", "nes"}}}, "bios"});
    if (!path) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    dependencies.core.request("machines.setBios", nlohmann::json{{"machine", machine}, {"path", *path}}, [done](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"canceled", false}, {"machine", reply.result.value("machine", nlohmann::json(nullptr))}}) : std::move(reply));
    });
  };

  handlers["machines.openBiosDirectory"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const std::string error = dependencies.biosDirectory.open();
    done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure("machines.biosDirectoryUnavailable", "无法打开 BIOS 目录：" + error));
  };

  return handlers;
}

}  // namespace ember::win32::entry
