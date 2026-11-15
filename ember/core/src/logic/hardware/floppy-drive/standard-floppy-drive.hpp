/*
 * ------------------------------------------------------------------------
 * 名称：standard-floppy-drive.hpp
 * 说明：软驱驱动器模块的实现：持有盘片镜像、磁头位置、马达、写保护与注入的故障。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：盘片的格式由镜像的大小推断（160、180、320、360、720、1200、1440、2880 KB），其他大小按 1.44MB 处理；
 *       “取出但保留镜像”用来模拟没有盘的故障，与真正取出（丢弃镜像）不同。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "contracts/hardware/floppy-drive.hpp"

namespace ember::logic::hardware {

/** 软驱驱动器。 */
class StandardFloppyDrive final : public contracts::hardware::FloppyDrive {
 public:
  /** @param exists 是否装了这个驱动器。 */
  explicit StandardFloppyDrive(bool exists = true) : exists_(exists) {}

  [[nodiscard]] bool exists() const override { return exists_; }
  [[nodiscard]] bool inserted() const override { return inserted_; }
  [[nodiscard]] bool motorRunning() const override { return motor_ && exists_; }
  void setMotor(bool on) override {
    if (motor_ == on) return;
    motor_ = on;
    if (onMotorChanged) onMotorChanged(on && exists_);
  }
  [[nodiscard]] bool writeProtected() const override { return writeProtected_; }
  [[nodiscard]] bool diskChanged() const override { return changed_; }
  void clearDiskChanged() override { changed_ = false; }
  [[nodiscard]] unsigned cylinder() const override { return cylinder_; }
  void setCylinder(unsigned cylinder) override { cylinder_ = cylinder; }
  [[nodiscard]] contracts::hardware::FloppyFormat format() const override { return inserted_ ? format_ : contracts::hardware::FloppyFormat{}; }
  [[nodiscard]] std::uint8_t readByte(std::size_t offset) const override { return inserted_ && offset < image_.size() ? image_[offset] : std::uint8_t{0}; }
  void writeByte(std::size_t offset, std::uint8_t value) override;
  [[nodiscard]] const contracts::hardware::FloppyFaults& faults() const override { return faults_; }

  void insertImage(std::vector<std::uint8_t> image, bool writeProtected, std::string source) override;
  void ejectImage() override;
  [[nodiscard]] const std::vector<std::uint8_t>& image() const override { return image_; }
  [[nodiscard]] const std::string& source() const override { return source_; }
  [[nodiscard]] bool modified() const override { return modified_; }
  void markSaved() override { modified_ = false; }
  void setWriteProtected(bool protect) override { writeProtected_ = protect; }
  contracts::hardware::FloppyFaults& mutableFaults() override { return faults_; }
  bool suspendMedia() override;
  bool restoreMedia() override;
  [[nodiscard]] bool mediaSuspended() const override { return suspended_; }

  /** 马达开关变化时调用（参数是马达是否在转）；由主机装配设置。 */
  std::function<void(bool)> onMotorChanged;

  /** 镜像大小对应的格式。 */
  [[nodiscard]] static contracts::hardware::FloppyFormat formatFor(std::size_t imageSize);

 private:
  bool exists_;
  bool inserted_{false};
  /** 盘片是否被 suspendMedia 暂时取出（镜像还在）。 */
  bool suspended_{false};
  bool motor_{false};
  bool writeProtected_{false};
  bool changed_{false};
  bool modified_{false};
  unsigned cylinder_{0};
  contracts::hardware::FloppyFormat format_{};
  contracts::hardware::FloppyFaults faults_;
  std::vector<std::uint8_t> image_;
  std::string source_;
};

}  // namespace ember::logic::hardware
