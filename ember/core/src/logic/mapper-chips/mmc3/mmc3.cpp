/*
 * ------------------------------------------------------------------------
 * 名称：mmc3.cpp
 * 说明：MMC3 芯片的实现，行为见 mmc3.hpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * ------------------------------------------------------------------------
 */

#include "logic/mapper-chips/mmc3/mmc3.hpp"

namespace ember::logic::mapper_chips {

Mmc3::Mmc3(contracts::hardware::IrqLine& irq) : irq_(irq), source_(irq.addSource()) {}

void Mmc3::reset() {
  selector_ = 0;
  registers_.fill(0);
  horizontalMirroring_ = false;
  resetIrq();
}

void Mmc3::resetIrq() {
  latch_ = 0;
  counter_ = 0;
  reload_ = false;
  enabled_ = false;
  a12High_ = false;
  lowAccesses_ = 0;
  irq_.set(source_, false);
}

void Mmc3::disableIrq() {
  enabled_ = false;
  irq_.set(source_, false);
}

void Mmc3::writeRegister(std::uint16_t address, std::uint8_t value) {
  const bool odd = (address & 1U) != 0;
  switch ((address >> 13) & 3U) {
    case 0:
      if (odd) registers_[selector_ & 7U] = value;
      else selector_ = value;
      break;
    case 1:
      if (!odd) horizontalMirroring_ = (value & 1U) != 0;
      break;
    case 2:
      if (odd) {
        counter_ = 0;
        reload_ = true;
      } else {
        latch_ = value;
      }
      break;
    default:
      if (odd) enabled_ = true;
      else disableIrq();
      break;
  }
}

unsigned Mmc3::programBank(std::uint16_t address) const {
  const bool swapped = (selector_ & 0x40U) != 0;
  switch ((address >> 13) & 3U) {
    case 0:
      return swapped ? SECOND_LAST_PROGRAM_BANK : (registers_[6] & 0x3FU);
    case 1:
      return registers_[7] & 0x3FU;
    case 2:
      return swapped ? (registers_[6] & 0x3FU) : SECOND_LAST_PROGRAM_BANK;
    default:
      return LAST_PROGRAM_BANK;
  }
}

unsigned Mmc3::characterBank(std::uint16_t address) const {
  const unsigned slot = (address >> 10) & 7U;
  // 选择位 7 把两个半区对调：先换算成不反转时的槽位。
  const unsigned index = (selector_ & 0x80U) != 0 ? (slot ^ 4U) : slot;
  // R0、R1 是 2K 页（页号取偶数，奇数的 1K 是它的后一半），R2-R5 是 1K 页。
  if (index < 4) return (registers_[index < 2 ? 0 : 1] & 0xFEU) + (index & 1U);
  return registers_[index - 2];
}

void Mmc3::onVideoAddress(std::uint16_t address) {
  if ((address & 0x1000U) != 0) {
    if (!a12High_ && lowAccesses_ >= A12_LOW_ACCESSES) clockCounter();
    a12High_ = true;
    lowAccesses_ = 0;
  } else {
    a12High_ = false;
    if (lowAccesses_ < A12_LOW_ACCESSES) ++lowAccesses_;
  }
}

void Mmc3::clockCounter() {
  if (counter_ == 0 || reload_) {
    counter_ = latch_;
    reload_ = false;
  } else {
    --counter_;
  }
  if (counter_ == 0 && enabled_) irq_.set(source_, true);
}

}  // namespace ember::logic::mapper_chips
