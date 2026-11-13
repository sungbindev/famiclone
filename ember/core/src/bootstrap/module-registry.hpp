/*
 * ------------------------------------------------------------------------
 * 名称：module-registry.hpp
 * 说明：模块登记时可用的公共能力：请求路由、事件发布、错误日志、时钟、应用版本、核心版本、用户数据目录、模块共享的服务与停止处理。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只在组装根中使用；各模块的登记函数用它创建自己的实现并登记请求。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bootstrap/module-services.hpp"
#include "platform/events/event-sink.hpp"
#include "platform/logging/error-log.hpp"
#include "platform/requests/request-router.hpp"
#include "platform/time/clock.hpp"

namespace ember::bootstrap {

/** 模块登记时可用的公共能力。 */
struct ModuleRegistry {
  platform::RequestRouter& requests;
  std::shared_ptr<platform::EventSink> events;
  std::shared_ptr<platform::ErrorLog> errors;
  std::shared_ptr<const platform::Clock> clock;
  /** 应用版本，由主进程在启动核心时传入。 */
  std::string appVersion;
  /** 核心版本，取自构建参数。 */
  std::string coreVersion;
  std::filesystem::path userDataPath;
  /** 预装的 mapper 插件目录；没有时为空。 */
  std::filesystem::path bundledPluginsPath;
  /** 模块之间共享的契约接口。 */
  ModuleServices& services;
  /** 核心停止时、等待请求结束之前按登记的相反顺序调用，用于停下模块自己的后台工作。 */
  std::vector<std::function<void()>>& stopHandlers;
};

/** 一个模块的登记项。 */
struct ModuleRegistration {
  /** 模块名。 */
  const char* name;
  /** 登记函数。 */
  void (*registerModule)(ModuleRegistry& registry);
};

}  // namespace ember::bootstrap
