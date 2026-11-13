/*
 * ------------------------------------------------------------------------
 * 名称：cartridges.cpp
 * 说明：卡带模块的登记：创建 NES 文件的装载实现，提供给后面的运行模块使用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：本阶段没有 cartridges.* 请求（打开卡带、卡带库属于以后的阶段），所以不登记请求；取用 mappers 模块的卡带工厂，须排在它之后登记。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/cartridges.hpp"

#include <memory>

#include "contracts/cartridges/cartridge-loader.hpp"
#include "contracts/mappers/mapper-factory.hpp"
#include "logic/cartridges/ines-cartridge-loader.hpp"

namespace ember::bootstrap {

void registerCartridges(ModuleRegistry& registry) {
  registry.services.provide<contracts::cartridges::CartridgeLoader>(
      std::make_shared<logic::cartridges::InesCartridgeLoader>(registry.services.require<contracts::mappers::MapperFactory>()));
}

}  // namespace ember::bootstrap
