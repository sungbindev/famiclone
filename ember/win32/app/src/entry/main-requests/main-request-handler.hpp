/*
 * ------------------------------------------------------------------------
 * 名称：main-request-handler.hpp
 * 说明：外壳自己处理的请求（Schema 里的 mainRequests）的处理函数类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：处理函数在界面线程调用，参数已确认是 JSON 对象；参数不合格时抛出 ParamError，业务失败经 done 给出带错误码的回复；
 *       需要等核心回复的请求在核心的回调里再调用 done，所以 done 可以晚于处理函数返回。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include "contracts/core-gateway/core-gateway.hpp"

namespace ember::win32::entry {

/** 外壳自己处理的请求的处理函数。 */
using MainRequestHandler = std::function<void(const nlohmann::json& params, contracts::CoreReplyHandler done)>;

/** 一个模块登记的处理函数，键是请求名“模块.操作”。 */
using MainRequestHandlers = std::map<std::string, MainRequestHandler, std::less<>>;

}  // namespace ember::win32::entry
