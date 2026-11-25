/*
 * ------------------------------------------------------------------------
 * 名称：apple-bitstream-decoder.hpp
 * 说明：位流格式苹果软盘镜像（.nib、.woz）的解码器：用 5.25 英寸软驱模块的磁道编解码把磁道解成扇区，供磁盘镜像管理只读查看。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：磁盘镜像管理（业务模块）不能引用硬件模块的实现，所以由组装根创建这个适配器注入；解码按 DOS 顺序排列扇区，磁道上解不出的扇区（防拷贝的特殊磁道等）填 0。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/diskimages/disk-file-system.hpp"

namespace ember::bootstrap {

/** 位流格式苹果软盘镜像的解码器。 */
class AppleBitstreamDecoder final : public contracts::diskimages::AppleBitstreamDecoder {
 public:
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> decode(std::span<const std::uint8_t> image, std::string_view extension) const override;
};

}  // namespace ember::bootstrap
