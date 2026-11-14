/*
 * ------------------------------------------------------------------------
 * 名称：input-requests.hpp
 * 说明：input 模块的请求登记：把 input.* 请求名映射到输入注入服务的方法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "contracts/input/input-service.hpp"
#include "platform/requests/request-router.hpp"

namespace ember::requests::input {

/** 登记 input 模块的请求。 */
void addInputRequests(platform::RequestRouter& router, std::shared_ptr<contracts::input::InputService> service);

}  // namespace ember::requests::input
