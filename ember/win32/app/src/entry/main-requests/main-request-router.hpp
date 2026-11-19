/*
 * ------------------------------------------------------------------------
 * 名称：main-request-router.hpp
 * 说明：请求路由：界面的请求按请求名分给外壳自己处理的请求，或转给核心；未登记的请求名、只能由外壳发起或只经调试服务开放的请求一律拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳的 entry/ipc/core 与主进程请求入口；请求名的类别取自由 Schema 生成的请求目录；
 *       外壳自己处理的请求，处理函数由各模块在组装根登记，请求名必须在目录里、不能重复登记；处理函数抛出的参数错误回复 core.invalidParams，其他异常回复 core.internal 并写日志。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/logging/error-logger.hpp"
#include "contracts/shell/request-broker.hpp"
#include "entry/main-requests/main-request-handler.hpp"

namespace ember::win32::entry {

/** 请求路由。 */
class MainRequestRouter : public contracts::RequestBroker {
 public:
  MainRequestRouter(contracts::CoreGateway& core, contracts::ErrorLogger& logger);

  /**
   * 登记一个模块的处理函数。
   * @throws std::logic_error 请求名不是外壳自己处理的请求，或已经登记过。
   */
  void add(MainRequestHandlers handlers);

  void request(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) override;

 private:
  void runHandler(const std::string& name, const MainRequestHandler& handler, const nlohmann::json& params, contracts::CoreReplyHandler done);

  contracts::CoreGateway& core_;
  contracts::ErrorLogger& logger_;
  MainRequestHandlers handlers_;
};

}  // namespace ember::win32::entry
