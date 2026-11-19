/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-handlers.cpp
 * 说明：cartridges 模块的外壳请求处理的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/cartridges/cartridge-handlers.hpp"

#include "entry/main-requests/dropped-path.hpp"
#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

namespace {

constexpr const char* kTitle = "打开卡带";
constexpr const char* kFilter = "NES 卡带";
constexpr const char* kDroppedNotFoundMessage = "拖入的文件已失效或不是 NES 卡带，请重新拖入";
constexpr const char* kRecentNotFoundMessage = "这个卡带已不在最近打开的清单里";
constexpr const char* kDroppedNotFound = "cartridges.droppedNotFound";
constexpr const char* kRecentNotFound = "cartridges.recentNotFound";

}  // namespace

void loadAndRememberCartridge(contracts::CoreGateway& core, contracts::RecentCartridgesService& recent, const std::string& path, contracts::CoreReplyHandler done) {
  recent.add(path);
  core.request("cartridges.load", nlohmann::json{{"path", path}}, [&recent, path, done = std::move(done)](contracts::CoreReply reply) {
    if (!reply.ok) recent.remove(path);
    done(std::move(reply));
  });
}

MainRequestHandlers createCartridgeHandlers(CartridgeDependencies dependencies) {
  MainRequestHandlers handlers;

  handlers["cartridges.open"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"droppedId"});
    const auto droppedId = reader.integer("droppedId", 1, INT64_MAX);
    std::optional<std::string> path;
    if (!droppedId) {
      path = dependencies.dialog.pickFile({kTitle, {{kFilter, {"nes"}}}, "cartridge"});
    } else {
      path = resolveDroppedPath(dependencies.dropped, *droppedId, {"nes"});
      if (!path) {
        done(contracts::CoreReply::failure(kDroppedNotFound, kDroppedNotFoundMessage));
        return;
      }
    }
    if (!path) {
      done(contracts::CoreReply::success({{"canceled", true}}));
      return;
    }
    loadAndRememberCartridge(dependencies.core, dependencies.recent, *path, [done](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"canceled", false}, {"loaded", std::move(reply.result)}}) : std::move(reply));
    });
  };

  handlers["cartridges.recent"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {});
    nlohmann::json items = nlohmann::json::array();
    for (const contracts::RecentCartridge& item : dependencies.recent.list()) items.push_back({{"id", item.id}, {"name", item.name}});
    done(contracts::CoreReply::success({{"items", std::move(items)}}));
  };

  handlers["cartridges.openRecent"] = [dependencies](const nlohmann::json& params, contracts::CoreReplyHandler done) {
    const ParamReader reader(params, {"id"});
    const std::int64_t id = reader.requiredInteger("id", 1, INT64_MAX);
    const auto path = dependencies.recent.resolve(id);
    if (!path) {
      done(contracts::CoreReply::failure(kRecentNotFound, kRecentNotFoundMessage));
      return;
    }
    loadAndRememberCartridge(dependencies.core, dependencies.recent, *path, [done](contracts::CoreReply reply) {
      done(reply.ok ? contracts::CoreReply::success({{"loaded", std::move(reply.result)}}) : std::move(reply));
    });
  };

  return handlers;
}

}  // namespace ember::win32::entry
