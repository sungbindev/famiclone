/*
 * ------------------------------------------------------------------------
 * 名称：disk2-codec.hpp
 * 说明：5.25 英寸软盘的磁道编码：按盘片格式把一个磁道的扇区编成一圈磁盘上的位序列，
 *       或反过来从位序列里解出扇区；并按镜像的大小与内容认出盘片格式。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：半字节编码是 6 加 2 编码，各格式相同：地址场 D5 AA 96、卷号、磁道、扇区、
 *       校验（各按 4 加 4 编码）、
 *       收尾 DE AA EB；数据场 D5 AA AD、343 个半字节（86 个辅助、256 个主体、1 个校验）、
 *       收尾 DE AA EB；
 *       磁盘上存的是位：一个半字节是 8 位，最高位是 1；同步字节 FF 后面跟两个 0 位（共 10 位），
 *       读出时 0 位被跳过，只出现 FF；
 *       格式（Format）给出每道扇区数、磁道数、逻辑扇区号与物理扇区号的对应表、卷号、
 *       一圈的位数与各段同步字节的个数；
 *       镜像里的扇区按“逻辑扇区号”排列（偏移 = (磁道 × 每道扇区数 + 逻辑扇区) × 256），
 *       磁盘上的扇区号是物理扇区号；
 *       已有两种格式族：金字塔 PEC-9588 的磁盘操作系统（每道 14 个扇区、40 道，
 *       对应表是 ROM 里 $BEB8 的交错表）、
 *       苹果 DOS 3.3 与 ProDOS 的磁盘（每道 16 个扇区、35 或 40 道，.dsk 按 DOS 顺序、
 *       .po 按 ProDOS 顺序排扇区，
 *       两种顺序由镜像内容认出）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ember::logic::hardware::disk2 {

/** 读头能走到的最大磁道数，也是磁道缓存的个数（任何格式的磁道数都不超过它）。 */
inline constexpr unsigned TRACK_COUNT = 40;
/** 扇区大小（字节）。 */
inline constexpr std::size_t SECTOR_SIZE = 256;
/** 每道扇区数的上限。 */
inline constexpr unsigned MAX_SECTORS_PER_TRACK = 16;

/** 一种盘片格式。 */
struct Format {
  /** 镜像里的磁道数。 */
  unsigned trackCount{0};
  /** 每道扇区数。 */
  unsigned sectorsPerTrack{0};
  /** 逻辑扇区号换成物理扇区号的对应表（前 sectorsPerTrack 项有效）。 */
  std::array<std::uint8_t, MAX_SECTORS_PER_TRACK> logicalToPhysical{};
  /** 地址场里的卷号。 */
  std::uint8_t volume{254};
  /** 一圈的位数。 */
  std::size_t trackBits{50000};
  /** 磁道开头的同步字节个数。 */
  std::size_t gapTrackStart{128};
  /** 地址场与数据场之间的同步字节个数。 */
  std::size_t gapAfterAddress{6};
  /**
   * 每个扇区后面的同步字节个数：要留够，学习机重写一个扇区时写得比原来慢，不能压到下一个地址场。
   */
  std::size_t gapAfterData{50};

  /** 一个磁道在镜像里的字节数。 */
  [[nodiscard]] constexpr std::size_t trackBytes() const { return sectorsPerTrack * SECTOR_SIZE; }
  /** 镜像的大小（字节）。 */
  [[nodiscard]] constexpr std::size_t imageSize() const { return trackCount * trackBytes(); }
};

/** 金字塔 PEC-9588 的盘片格式：40 道、每道 14 个扇区，镜像 143360 字节。 */
inline constexpr Format PYRAMID_FORMAT{40, 14, {0, 11, 9, 7, 5, 3, 1, 12, 10, 8, 6, 4, 2, 13, 0, 0}, 254, 50000, 128, 6, 50};

/**
 * 苹果 DOS 顺序的盘片：每道 16 个扇区，磁道数由镜像大小定（35 道 143360 字节，40 道 163840 字节）。
 */
[[nodiscard]] constexpr Format appleDosFormat(unsigned trackCount) {
  return Format{trackCount, 16, {0, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 15}, 254, 51200, 16, 6, 22};
}

/** 苹果 ProDOS 顺序的盘片：每道 16 个扇区，磁道数由镜像大小定。 */
[[nodiscard]] constexpr Format appleProdosFormat(unsigned trackCount) {
  return Format{trackCount, 16, {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15}, 254, 51200, 16, 6, 22};
}

/** 从镜像认出盘片格式的函数；认不出（大小不对）返回空。 */
using FormatChooser = std::optional<Format> (*)(std::span<const std::uint8_t> image);

/** 金字塔：大小正好是 143360 字节就是 PYRAMID_FORMAT。 */
[[nodiscard]] std::optional<Format> pyramidFormatOf(std::span<const std::uint8_t> image);

/**
 * 苹果：大小是 143360（35 道）或 163840（40 道）字节，再按内容认出扇区顺序：
 * 第 17 道第 0 扇区（DOS 顺序下的偏移 $11000）是 DOS 3.x 的卷目录表就是 DOS 顺序；
 * 块 2（偏移 $400）是 ProDOS 的卷目录头就是 ProDOS 顺序；两者都不是时按 DOS 顺序。
 */
[[nodiscard]] std::optional<Format> appleFormatOf(std::span<const std::uint8_t> image);

/** 从磁盘上解出的一个扇区。 */
struct DecodedSector {
  /** 地址场里的磁道号。 */
  unsigned track{0};
  /** 地址场里的物理扇区号。 */
  unsigned physical{0};
  /** 对应的逻辑扇区号（镜像里的位置）。 */
  unsigned logical{0};
  std::array<std::uint8_t, SECTOR_SIZE> data{};
};

/**
 * 把一个磁道编成一圈位（每个元素是 0 或 1，长度 format.trackBits）。
 * @param format 盘片格式。
 * @param track 磁道号（写进地址场）。
 * @param sectors 这个磁道的全部逻辑扇区，长度必须是 format.trackBytes()。
 */
[[nodiscard]] std::vector<std::uint8_t> encodeTrack(const Format& format, unsigned track, std::span<const std::uint8_t> sectors);

/**
 * 从一圈位里解出地址场与数据场都完好的扇区。
 * 地址场校验不对、没有数据场、数据校验不对的扇区丢弃；同一个扇区出现多次时取后面的。
 * @param format 盘片格式。
 * @param bits 一圈位（每个元素是 0 或 1），首尾相接。
 */
[[nodiscard]] std::vector<DecodedSector> decodeTrack(const Format& format, std::span<const std::uint8_t> bits);

}  // namespace ember::logic::hardware::disk2
