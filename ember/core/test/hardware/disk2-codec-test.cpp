/*
 * ------------------------------------------------------------------------
 * 名称：disk2-codec-test.cpp
 * 说明：盘片格式识别的测试：金字塔认 143360 字节的扇区镜像与 232960、266240 字节的 .nib 原始磁道镜像，
 *       其余大小认不出；.nib 的磁道由半字节展开、写回时读出的半字节与放入的一致。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-27
 * 备注：.nib 内容用金字塔格式把数据全零的磁道编出来再去掉同步字节的 0 位（与 machines/pyramid/tools/pyramid-blank-nib.mjs 相同），
 *       不依赖参考文件。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "logic/hardware/disk2/disk2-codec.hpp"

namespace {

namespace disk2 = ember::logic::hardware::disk2;

/** 用金字塔格式编出数据全零的一圈位，再换成 .nib 的一圈半字节（6656 个）。 */
std::vector<std::uint8_t> blankNibbleTrack(unsigned track) {
  const std::vector<std::uint8_t> sectors(disk2::PYRAMID_FORMAT.trackBytes(), 0);
  const std::vector<std::uint8_t> bits = disk2::encodeTrack(disk2::PYRAMID_FORMAT, track, sectors);
  return disk2::bitsToNibbles(bits, disk2::NIBBLE_TRACK_BYTES);
}

/** 35 道数据全零的金字塔 .nib 镜像（232960 字节）。 */
std::vector<std::uint8_t> blankNibImage(unsigned trackCount) {
  std::vector<std::uint8_t> image;
  image.reserve(static_cast<std::size_t>(trackCount) * disk2::NIBBLE_TRACK_BYTES);
  for (unsigned track = 0; track < trackCount; ++track) {
    const std::vector<std::uint8_t> nibbles = blankNibbleTrack(track);
    image.insert(image.end(), nibbles.begin(), nibbles.end());
  }
  return image;
}

TEST(Disk2Codec, PyramidAcceptsTheSectorImageAndBothNibbleSizes) {
  const std::vector<std::uint8_t> sectorImage(disk2::PYRAMID_FORMAT.imageSize(), 0);
  const auto sector = disk2::pyramidFormatOf(sectorImage);
  ASSERT_TRUE(sector.has_value());
  EXPECT_EQ(sector->layout, disk2::Layout::sector);
  EXPECT_EQ(sector->trackCount, 40U);
  EXPECT_EQ(sector->sectorsPerTrack, 14U);

  const auto nib35 = disk2::pyramidFormatOf(blankNibImage(35));
  ASSERT_TRUE(nib35.has_value());
  EXPECT_EQ(nib35->layout, disk2::Layout::nibble);
  EXPECT_EQ(nib35->trackCount, 35U);
  EXPECT_EQ(nib35->sectorsPerTrack, disk2::PYRAMID_FORMAT.sectorsPerTrack) << "每道扇区数按金字塔的 14，不是半字节数折算的 26";
  EXPECT_EQ(nib35->trackBits, disk2::NIBBLE_TRACK_BYTES * 8);
  EXPECT_EQ(nib35->trackBytes(), disk2::NIBBLE_TRACK_BYTES) << ".nib 每道 6656 字节";
  EXPECT_EQ(nib35->imageSize(), 232960U);

  const auto nib40 = disk2::pyramidFormatOf(blankNibImage(40));
  ASSERT_TRUE(nib40.has_value());
  EXPECT_EQ(nib40->layout, disk2::Layout::nibble);
  EXPECT_EQ(nib40->trackCount, 40U);

  EXPECT_FALSE(disk2::pyramidFormatOf(std::vector<std::uint8_t>(1000, 0)).has_value()) << "别的大小认不出";
}

TEST(Disk2Codec, PyramidNibbleTrackDecodesBackToZeroSectors) {
  const std::vector<std::uint8_t> nibbles = blankNibbleTrack(17);
  const std::vector<std::uint8_t> bits = disk2::nibblesToBits(nibbles);
  const auto format = disk2::pyramidFormatOf(blankNibImage(35));
  ASSERT_TRUE(format.has_value());
  const std::vector<disk2::DecodedSector> sectors = disk2::decodeTrack(*format, bits);
  ASSERT_EQ(sectors.size(), disk2::PYRAMID_FORMAT.sectorsPerTrack);
  for (const disk2::DecodedSector& sector : sectors) {
    EXPECT_EQ(sector.track, 17U);
    EXPECT_LT(sector.logical, disk2::PYRAMID_FORMAT.sectorsPerTrack);
    for (const std::uint8_t byte : sector.data) EXPECT_EQ(byte, 0U);
  }
  // 写回：从展开的位里读出的半字节与放入的一致。
  EXPECT_EQ(disk2::bitsToNibbles(bits, disk2::NIBBLE_TRACK_BYTES), nibbles);
}

}  // namespace