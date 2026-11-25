/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-loading.cpp
 * 说明：加载卡带并记入最近打开的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-26
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "entry/main-requests/cartridge-loading.hpp"

namespace ember::win32::entry {

void loadAndRememberCartridge(contracts::CoreGateway& core, contracts::RecentCartridgesService& recent, const std::string& path, contracts::CoreReplyHandler done) {
  recent.add(path);
  core.request("cartridges.load", nlohmann::json{{"path", path}}, [&recent, path, done = std::move(done)](contracts::CoreReply reply) {
    if (!reply.ok) recent.remove(path);
    done(std::move(reply));
  });
}

}  // namespace ember::win32::entry
