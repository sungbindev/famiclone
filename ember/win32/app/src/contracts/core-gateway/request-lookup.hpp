/*
 * ------------------------------------------------------------------------
 * 名称：request-lookup.hpp
 * 说明：按请求名查询请求的类别：核心请求（界面可发起、只能由外壳发起、只经调试服务开放）与外壳自己处理的请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：数据来自由 Schema 生成的 generated/request-catalog.hpp，不手写请求名。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <optional>
#include <string_view>

#include "contracts/core-gateway/generated/request-catalog.hpp"

namespace ember::win32::contracts {

/** 核心请求的类别；不是核心请求时为空。 */
[[nodiscard]] inline std::optional<RequestKind> coreRequestKind(std::string_view name) {
  const auto found = std::find_if(std::begin(kCoreRequests), std::end(kCoreRequests), [name](const CatalogEntry& entry) { return entry.name == name; });
  return found == std::end(kCoreRequests) ? std::nullopt : std::optional<RequestKind>(found->kind);
}

/** 是不是外壳自己处理的请求。 */
[[nodiscard]] inline bool isShellRequest(std::string_view name) { return std::find(std::begin(kShellRequests), std::end(kShellRequests), name) != std::end(kShellRequests); }

}  // namespace ember::win32::contracts
