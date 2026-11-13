/*
 * ------------------------------------------------------------------------
 * 名称：output-requests.cpp
 * 说明：output 模块的请求登记实现：校验参数，调用输出设置服务，返回结果。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "requests/output/output-requests.hpp"

namespace ember::requests::output {

namespace contract = ::ember::contracts::output;

void addOutputRequests(platform::RequestRouter& router, std::shared_ptr<contract::OutputService> service) {
  router.add("output", contract::OUTPUT_SET_VOLUME_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseOutputSetVolumeParams(params, "params");
    return nlohmann::json(service->setVolume(parsed.volume, parsed.muted));
  });
}

}  // namespace ember::requests::output
