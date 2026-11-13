/*
 * ------------------------------------------------------------------------
 * 名称：mappers.cpp
 * 说明：mapper 插件模块的登记：创建插件登记（预装目录、用户插件目录、主机型号目录），登记 mappers.* 请求，并把插件管理与卡带工厂提供给其他模块。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：主机型号目录列出程序里已有的主机型号；目前只有 fc；增加主机型号时在这里登记。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/mappers.hpp"

#include <memory>

#include "contracts/mappers/mapper-factory.hpp"
#include "contracts/mappers/mapper-service.hpp"
#include "infra/mappers/plugin-registry.hpp"
#include "requests/mappers/mappers-requests.hpp"

namespace ember::bootstrap {

void registerMappers(ModuleRegistry& registry) {
  infra::mappers::PluginRegistryOptions options;
  options.bundledDirectory = registry.bundledPluginsPath;
  options.userDirectory = registry.userDataPath / "plugins";
  options.hosts.hosts = {"fc"};
  options.errors = registry.errors;
  options.events = registry.events;
  auto plugins = std::make_shared<infra::mappers::PluginRegistry>(std::move(options));
  registry.services.provide<contracts::mappers::MapperService>(plugins);
  registry.services.provide<contracts::mappers::MapperFactory>(plugins);
  requests::mappers::addMapperRequests(registry.requests, plugins);
}

}  // namespace ember::bootstrap
