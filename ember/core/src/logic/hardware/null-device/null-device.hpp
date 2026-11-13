/*
 * ------------------------------------------------------------------------
 * 名称：null-device.hpp
 * 说明：空设备：占住一段总线地址但什么也不做，读不驱动数据线（开放总线），写被忽略。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用于还没有实现的芯片的占位（如 fc 主机型号里的 APU，$4000-$4015），实现后在装配处替换成真正的模块。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

#include "contracts/hardware/device.hpp"

namespace ember::logic::hardware {

/** 空设备。 */
class NullDevice final : public contracts::hardware::Device {
 public:
  contracts::hardware::BusRead read(std::uint16_t /*address*/) override { return contracts::hardware::releaseBus(); }

  void write(std::uint16_t /*address*/, std::uint8_t /*value*/) override {}

  void reset(bool /*power*/) override {}

  void onEvent(contracts::hardware::MasterTime /*when*/, std::uint32_t /*tag*/) override {}
};

}  // namespace ember::logic::hardware
