/*
 * ------------------------------------------------------------------------
 * 名称：floppy-bitstream-codec.hpp
 * 说明：位流格式软盘镜像（.nib、.woz）的编解码器：用 5.25 英寸软驱模块的磁道编解码把磁道解成扇区供媒体库只读查看，或把金字塔扇区镜像编成 .nib 供新建空白盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：媒体库（业务模块）不能引用硬件模块的实现，所以由组装根创建这个适配器注入；苹果与金字塔的盘用同一套半字节编码，
 *       差别只在盘片格式参数（每道扇区数、交错表、一圈位数）；苹果的解码按 DOS 顺序排列扇区，
 *       金字塔的按逻辑扇区号排列（每道 14 个扇区），磁道上解不出的扇区（防拷贝的特殊磁道等）填 0。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/filemanager/disk-file-system.hpp"

namespace ember::bootstrap {

/** 位流格式软盘镜像的编解码器。 */
class FloppyBitstreamCodec final : public contracts::filemanager::FloppyBitstreamCodec {
 public:
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> decode(std::span<const std::uint8_t> image, std::string_view extension) const override;
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> decodePyramid(std::span<const std::uint8_t> image, std::string_view extension) const override;
  [[nodiscard]] bool isPyramidDisk(std::span<const std::uint8_t> image, std::string_view extension) const override;
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> encodePyramidNib(std::span<const std::uint8_t> image) const override;
};

}  // namespace ember::bootstrap
