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

#include <chrono>
#include <future>

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
  router.add("runtime", contract::RUNTIME_RUN_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken& token) {
    const auto parsed = contract::parseRuntimeRunParams(params, "params");
    auto outcome = service->startRun(parsed.frames);
    contract::RuntimeRunResult result;
    if (parsed.wait.value_or(false)) {
      // 等运行结束；请求被取消（连接断开、服务停止）时打断运行，结果给出已经跑的部分。
      while (outcome.wait_for(std::chrono::milliseconds(20)) != std::future_status::ready) {
        if (token.isCancelled()) {
          service->cancelRun();
          break;
        }
      }
      const auto& finished = outcome.get();
      result.frames = finished.frames;
      result.cycles = finished.cycles;
      result.stopped = finished.stopped;
    }
    const auto status = service->status();
    result.state = contract::RuntimeState{status.power, status.mode};
    return nlohmann::json(result);
  });
  router.add("runtime", contract::RUNTIME_SET_FRAME_EVENTS_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseRuntimeSetFrameEventsParams(params, "params");
    service->setFrameEventInterval(static_cast<std::uint32_t>(parsed.frames));
    return nlohmann::json(contract::RuntimeSetFrameEventsResult{parsed.frames});
  });
}

}  // namespace ember::requests::runtime
