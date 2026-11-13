/*
 * ------------------------------------------------------------------------
 * 名称：runtime-requests.cpp
 * 说明：runtime 模块的请求登记实现：校验参数，调用运行服务，返回结果。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "requests/runtime/runtime-requests.hpp"

namespace ember::requests::runtime {

namespace contract = ::ember::contracts::runtime;

void addRuntimeRequests(platform::RequestRouter& router, std::shared_ptr<contract::RuntimeService> service) {
  router.add("runtime", contract::RUNTIME_STATUS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseRuntimeStatusParams(params, "params"));
    return nlohmann::json(service->status());
  });
  router.add("runtime", contract::RUNTIME_POWER_ON_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseRuntimePowerOnParams(params, "params");
    return nlohmann::json(contract::RuntimePowerOnResult{service->powerOn(parsed.resume.value_or(false))});
  });
  router.add("runtime", contract::RUNTIME_POWER_OFF_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseRuntimePowerOffParams(params, "params"));
    return nlohmann::json(contract::RuntimePowerOffResult{service->powerOff()});
  });
  router.add("runtime", contract::RUNTIME_RESET_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseRuntimeResetParams(params, "params");
    const auto kind = parsed.kind == "soft" ? contract::ResetKind::Soft : contract::ResetKind::Hard;
    return nlohmann::json(contract::RuntimeResetResult{service->reset(kind)});
  });
  router.add("runtime", contract::RUNTIME_PAUSE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseRuntimePauseParams(params, "params"));
    return nlohmann::json(contract::RuntimePauseResult{service->pause()});
  });
  router.add("runtime", contract::RUNTIME_RESUME_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseRuntimeResumeParams(params, "params"));
    return nlohmann::json(contract::RuntimeResumeResult{service->resume()});
  });
}

}  // namespace ember::requests::runtime
