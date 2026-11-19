/*
 * ------------------------------------------------------------------------
 * 名称：preference-handlers.cpp
 * 说明：preferences 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/preferences/preference-handlers.hpp"

#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

MainRequestHandlers createPreferenceHandlers(PreferenceDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["preferences.pickDirectory"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    const auto path = dependencies.dialog.pickDirectory({"选择文件夹", "folder"});
    done(path ? contracts::CoreReply::success({{"canceled", false}, {"path", *path}}) : contracts::CoreReply::success({{"canceled", true}}));
  };

  return handlers;
}

}  // namespace ember::win32::entry
