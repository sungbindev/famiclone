/*
 * ------------------------------------------------------------------------
 * 名称：main-request-router.cpp
 * 说明：请求路由的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：错误码与核心一致（core.unknownRequest、core.invalidParams、core.internal）。
 * ------------------------------------------------------------------------
 */

#include "entry/main-requests/main-request-router.hpp"

#include <exception>
#include <stdexcept>

#include "contracts/core-gateway/request-lookup.hpp"
#include "entry/main-requests/param-reader.hpp"

namespace ember::win32::entry {

namespace {

constexpr const char* kUnknownRequest = "core.unknownRequest";
constexpr const char* kInvalidParams = "core.invalidParams";
constexpr const char* kInternal = "core.internal";

contracts::CoreReply unknownReply(const std::string& name) { return contracts::CoreReply::failure(kUnknownRequest, "没有这个请求：" + name); }

}  // namespace

MainRequestRouter::MainRequestRouter(contracts::CoreGateway& core, contracts::ErrorLogger& logger) : core_(core), logger_(logger) {}

void MainRequestRouter::add(MainRequestHandlers handlers) {
  for (auto& [name, handler] : handlers) {
    if (!contracts::isShellRequest(name)) throw std::logic_error("不是外壳处理的请求：" + name);
    if (!handlers_.emplace(name, std::move(handler)).second) throw std::logic_error("请求重复登记：" + name);
  }
}

void MainRequestRouter::runHandler(const std::string& name, const MainRequestHandler& handler, const nlohmann::json& params, contracts::CoreReplyHandler done) {
  try {
    handler(params, done);
  } catch (const ParamError& error) {
    // 处理函数抛出时还没有调用过 done（参数在处理函数开头校验），所以这里可以回复
    done(contracts::CoreReply::failure(kInvalidParams, std::string("请求参数不正确：") + error.what()));
  } catch (const std::exception& error) {
    logger_.write({std::nullopt, kInternal, std::string("外壳请求失败：") + error.what(), nlohmann::json{{"name", name}}});
    done(contracts::CoreReply::failure(kInternal, "请求失败，详情已写入日志"));
  }
}

void MainRequestRouter::request(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) {
  if (contracts::isShellRequest(name)) {
    const auto found = handlers_.find(name);
    if (found == handlers_.end()) {
      done(unknownReply(name));
      return;
    }
    runHandler(name, found->second, params, std::move(done));
    return;
  }
  const auto kind = contracts::coreRequestKind(name);
  if (!kind || *kind != contracts::RequestKind::Ui) {
    done(unknownReply(name));
    return;
  }
  core_.request(name, std::move(params), std::move(done));
}

}  // namespace ember::win32::entry
