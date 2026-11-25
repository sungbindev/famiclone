/*
 * ------------------------------------------------------------------------
 * 名称：disk2-codec.hpp
 * 说明：金字塔学习机 5.25 英寸软盘的磁道编码：把一个磁道的 14 个扇区编成一圈磁盘上的位序列，
 *       或反过来从位序列里解出扇区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：格式由金字塔 PEC-9588 的磁盘操作系统（ROM 里的 RWTS）定：每道 14 个扇区、每扇区
 *       256 字节、共 40 道，镜像大小 143360 字节；
 *       半字节编码是 6 加 2 编码：地址场 D5 AA 96、卷号、磁道、扇区、校验（各按 4 加 4 编码）、
 *       收尾 DE AA EB；数据场 D5 AA AD、343 个半字节（86 个辅助、256 个主体、1 个校验）、
 *       收尾 DE AA EB；
 *       磁盘上存的是位：每个位占 4 微秒，一圈 50000 位（转速 300 转每分钟）；一个半字节是 8 位，
 *       最高位是 1；同步字节 FF 后面跟两个 0 位（共 10 位），读出时 0 位被跳过，只出现 FF；
 *       镜像里的扇区按“逻辑扇区号”排列（偏移 = (磁道 × 14 + 逻辑扇区) × 256），磁盘上的扇区号
 *       是物理扇区号，两者的对应表就是 ROM 里 $BEB8 的交错表。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ember::logic::hardware::disk2 {

/** 磁道数（镜像里的）。 */
inline constexpr unsigned TRACK_COUNT = 40;
/** 每道扇区数。 */
inline constexpr unsigned SECTORS_PER_TRACK = 14;
/** 扇区大小（字节）。 */
inline constexpr std::size_t SECTOR_SIZE = 256;
/** 一个磁道在镜像里的字节数。 */
inline constexpr std::size_t TRACK_BYTES = SECTORS_PER_TRACK * SECTOR_SIZE;
/** 镜像的大小（字节）。 */
inline constexpr std::size_t IMAGE_SIZE = TRACK_COUNT * TRACK_BYTES;
/** 一圈的位数。 */
inline constexpr std::size_t TRACK_BITS = 50000;
/** 地址场里的卷号。 */
inline constexpr std::uint8_t VOLUME_NUMBER = 254;

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
 * 把一个磁道编成一圈位（每个元素是 0 或 1，长度 TRACK_BITS）。
 * @param track 磁道号（写进地址场）。
 * @param sectors 这个磁道的 14 个逻辑扇区，长度必须是 TRACK_BYTES。
 */
[[nodiscard]] std::vector<std::uint8_t> encodeTrack(unsigned track, std::span<const std::uint8_t> sectors);

/**
 * 从一圈位里解出地址场与数据场都完好的扇区。
 * 地址场校验不对、没有数据场、数据校验不对的扇区丢弃；同一个扇区出现多次时取后面的。
 * @param bits 一圈位（每个元素是 0 或 1），首尾相接。
 */
[[nodiscard]] std::vector<DecodedSector> decodeTrack(std::span<const std::uint8_t> bits);

}  // namespace ember::logic::hardware::disk2
