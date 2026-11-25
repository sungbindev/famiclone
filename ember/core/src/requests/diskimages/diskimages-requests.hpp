/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-requests.hpp
 * 说明：diskimages 模块的请求登记：把 diskimages.* 请求名映射到磁盘镜像管理服务的方法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>

#include "contracts/diskimages/diskimages-service.hpp"
#include "platform/requests/request-router.hpp"

namespace ember::requests::diskimages {

/** 登记 diskimages 模块的请求。 */
void addDiskimagesRequests(platform::RequestRouter& router, std::shared_ptr<contracts::diskimages::DiskImageService> service);

}  // namespace ember::requests::diskimages
