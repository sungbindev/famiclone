/*
 * ------------------------------------------------------------------------
 * 名称：mapper-circuits.cpp
 * 说明：组装根：电路目录的实现，见 mapper-circuits.hpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/mapper-circuits.hpp"

#include <string>

#include "contracts/mappers/mapper-error.hpp"
#include "logic/hardware/mapper-chips/mmc3/mmc3.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::bootstrap {

bool StandardCircuitCatalog::knows(std::string_view type) const { return type == "mmc3"; }

std::unique_ptr<contracts::hardware::MapperCircuit> StandardCircuitCatalog::create(std::string_view type, contracts::hardware::IrqLine& irq, const contracts::mappers::CircuitOptions& options) const {
  if (type == "mmc3") return std::make_unique<logic::hardware::mapper_chips::Mmc3>(irq, options.revA);
  throw shared::CoreError(contracts::mappers::ERROR_INVALID_PACKAGE, "没有这种电路：" + std::string(type));
}

}  // namespace ember::bootstrap
