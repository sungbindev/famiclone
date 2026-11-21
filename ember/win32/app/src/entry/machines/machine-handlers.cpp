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

  handlers["machines.openRom"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"machine"});
    const std::string machine = reader.requiredString("machine");
    const auto path = dependencies.dialog.pickFile({"添加 ROM", {{"ROM 文件", {"bin", "nes"}}}, "rom"});
    if (!path) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    dependencies.core.request("machines.setRom", nlohmann::json{{"machine", machine}, {"path", *path}}, [done](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"canceled", false}, {"machine", reply.result.value("machine", nlohmann::json(nullptr))}}) : std::move(reply));
    });
  };

  handlers["machines.openRomDirectory"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const std::string error = dependencies.romDirectory.open();
    done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure("machines.romDirectoryUnavailable", "无法打开 ROM 目录：" + error));
  };

  return handlers;
}

}  // namespace ember::win32::entry
