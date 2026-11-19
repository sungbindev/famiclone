/*
 * ------------------------------------------------------------------------
 * 名称：request-broker.hpp
 * 说明：请求中转的抽象：界面把请求交给它，它按请求名分给外壳自己处理的请求或核心，回复经回调交回界面线程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应 Electron 外壳里界面经预加载脚本发给主进程的请求入口（ipc/core 与 ipc/<模块>）；未登记的请求名、只能由外壳发起或只经调试服务开放的请求一律拒绝。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "contracts/core-gateway/core-gateway.hpp"

namespace ember::win32::contracts {

/** 请求中转。 */
class RequestBroker {
 public:
  virtual ~RequestBroker() = default;

  /** 处理一次来自界面的请求；回复经 done 在界面线程交回。 */
  virtual void request(const std::string& name, nlohmann::json params, CoreReplyHandler done) = 0;
};

}  // namespace ember::win32::contracts
