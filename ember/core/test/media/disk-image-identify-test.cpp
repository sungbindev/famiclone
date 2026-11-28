/*
 * ------------------------------------------------------------------------
 * 名称：disk-image-identify-test.cpp
 * 说明：磁盘镜像识别的测试：35 道的盘存放在 40 道容器里仍按 DOS 3.3 认出、Apple Pascal 盘能列目录与导出、认不出的 140K 镜像标“未识别”而不冒充金字塔。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-28
 * 备注：镜像都在测试内构造，不依赖外部盘片。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "logic/filemanager/apple-dos-file-system.hpp"
#include "logic/filemanager/apple-image.hpp"
#include "logic/filemanager/disk-image-opener.hpp"
#include "logic/filemanager/disk-text.hpp"

namespace {

using namespace ember::logic::filemanager;

constexpr std::size_t SECTOR = 256;
constexpr std::size_t TRACK = 16 * SECTOR;

TEST(DiskImageIdentify, A35TrackDosDiskInsideA40TrackContainerIsStillDos33) {
  std::vector<std::uint8_t> linear(35 * TRACK, 0);
  AppleDosFileSystem::format(linear, 35);
  std::vector<std::uint8_t> image(40 * TRACK, 0);
  appleFromLinear(image, linear, AppleOrder::Dos, AppleOrder::Dos, 35);
  const AppleIdentity identity = identifyApple(image, "dsk");
  EXPECT_EQ(identity.kind, AppleKind::Dos33);
  EXPECT_EQ(identity.tracks, 35U) << "道数按卷目录表里的记载，不是容器大小";
  const IdentifiedImage opened = identifyImage(image, "dsk", nullptr);
  ASSERT_NE(opened.fileSystem, nullptr);
  EXPECT_EQ(opened.formatId, "appledos33");
  EXPECT_EQ(opened.fileSystem->space().capacity, 35U * TRACK);
}

TEST(DiskImageIdentify, ApplePascalDiskListsAndExportsItsFiles) {
  std::vector<std::uint8_t> linear(35 * TRACK, 0);
  std::uint8_t* header = linear.data() + 2 * 512;
  writeLe16(header, 0);
  writeLe16(header + 2, 6);
  writeLe16(header + 4, 0);
  header[6] = 4;
  header[7] = 'T'; header[8] = 'E'; header[9] = 'S'; header[10] = 'T';
  writeLe16(header + 14, 280);
  writeLe16(header + 16, 1);
  std::uint8_t* entry = header + 22;
  writeLe16(entry, 0);      // 最后一块占满
  writeLe16(entry + 2, 0);  // 没有日期
  writeLe16(entry + 4, 6);  // 起始块
  writeLe16(entry + 6, 8);  // 结束块（不含）
  writeLe16(entry + 8, 5);  // DATA
  entry[10] = 9;
  const char name[] = "HELLO.TXT";
  for (int i = 0; i < 9; ++i) entry[11 + i] = static_cast<std::uint8_t>(name[i]);
  for (std::size_t i = 0; i < 2 * 512; ++i) linear[6 * 512 + i] = static_cast<std::uint8_t>(i % 251);
  std::vector<std::uint8_t> image(35 * TRACK, 0);
  appleFromLinear(image, linear, AppleOrder::Dos, AppleOrder::ProDos, 35);

  const IdentifiedImage opened = identifyImage(image, "dsk", nullptr);
  ASSERT_NE(opened.fileSystem, nullptr);
  EXPECT_EQ(opened.formatId, "pascal");
  EXPECT_FALSE(opened.fileSystem->traits().writable);
  const auto entries = opened.fileSystem->list({});
  ASSERT_EQ(entries.size(), 1U);
  EXPECT_EQ(entries[0].name, "HELLO.TXT");
  EXPECT_EQ(entries[0].type, "DATA");
  EXPECT_EQ(entries[0].size, 1024);
  EXPECT_TRUE(entries[0].modified.empty());
  const auto file = opened.fileSystem->readFile({"HELLO.TXT"});
  ASSERT_EQ(file.data.size(), 1024U);
  EXPECT_EQ(file.data[0], 0);
  EXPECT_EQ(file.data[251], 0);
  EXPECT_EQ(file.data[252], 1);
}

TEST(DiskImageIdentify, AnUnrecognizable140KImageIsUnknownNotPyramid) {
  std::vector<std::uint8_t> image(35 * TRACK, 0xAA);
  const IdentifiedImage opened = identifyImage(image, "dsk", nullptr);
  EXPECT_EQ(opened.fileSystem, nullptr);
  EXPECT_EQ(opened.formatId, "unknown");
  EXPECT_EQ(opened.formatName, "未识别");
}

}  // namespace