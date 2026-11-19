/*
 * ------------------------------------------------------------------------
 * 名称：system-handlers.cpp
 * 说明：system 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/system/system-handlers.hpp"

#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

namespace {

/** 升级状态转成 UpdateStatus 的 JSON。 */
nlohmann::json statusJson(const contracts::UpdateStatus& status) {
  nlohmann::json json{{"state", status.state}};
  if (status.version) json["version"] = *status.version;
  if (status.message) json["message"] = *status.message;
  return json;
}

}  // namespace

MainRequestHandlers createSystemHandlers(SystemDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["system.openLogDirectory"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const std::string error = dependencies.logDirectory.open();
    done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure("system.logDirectoryUnavailable", "无法打开日志目录：" + error));
  };

  handlers["system.openHelp"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"page"});
    const auto page = reader.enumeration("page", {"changelog"});
    const std::string error = dependencies.help.open(page);
    done(error.empty() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure("system.helpUnavailable", "无法打开帮助文档：" + error));
  };

  handlers["system.updateStatus"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    done(contracts::CoreReply::success({{"status", statusJson(dependencies.updater.status())}}));
  };

  handlers["system.checkUpdate"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    done(contracts::CoreReply::success({{"status", statusJson(dependencies.updater.check())}}));
  };

  handlers["system.installUpdate"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    done(dependencies.updater.install() ? contracts::CoreReply::success(nlohmann::json::object()) : contracts::CoreReply::failure("system.updateNotReady", "还没有下载好的新版本"));
  };

  return handlers;
}

}  // namespace ember::win32::entry
