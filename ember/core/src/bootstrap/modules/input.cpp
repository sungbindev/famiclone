/*
 * ------------------------------------------------------------------------
 * 名称：input.cpp
 * 说明：输入注入模块的登记：创建输入注入服务，登记 input.* 请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/input.hpp"

#include <memory>

#include "contracts/input/input-service.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "logic/input/local-input-service.hpp"
#include "requests/input/input-requests.hpp"

namespace ember::bootstrap {

void registerInput(ModuleRegistry& registry) {
  auto runtime = registry.services.require<contracts::runtime::RuntimeService>();
  auto service = std::make_shared<logic::input::LocalInputService>(std::move(runtime));
  registry.services.provide<contracts::input::InputService>(service);
  requests::input::addInputRequests(registry.requests, service);
}

}  // namespace ember::bootstrap
