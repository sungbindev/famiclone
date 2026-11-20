/*
 * ------------------------------------------------------------------------
 * 名称：ines-parser-test.cpp
 * 说明：iNES 解析的测试：iNES 1.0 与 NES 2.0 的字段、变体取值规则、训练器、镜像、校验值，以及魔数不对、文件被截断、不支持的格式等错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：真实文件头取自原版 SB2K_BIOS.NES（mapper 171，第 9 字节为 02，变体 1）。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/cartridges/cartridge-error.hpp"
#include "logic/cartridges/crc32.hpp"
#include "logic/cartridges/ines-parser.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace cart = ember::contracts::cartridges;
using ember::contracts::mappers::Mirroring;
using ember::logic::cartridges::crc32;
using ember::logic::cartridges::crc32Continue;
using ember::logic::cartridges::parseInes;

/** 构造一个文件：16 字节文件头（由参数给出前 10 个字节）、可选的训练器、PRG 与 CHR。数据按位置取值，便于核对。 */
std::vector<std::uint8_t> makeFile(std::uint8_t prgUnits, std::uint8_t chrUnits, std::uint8_t flags6 = 0, std::uint8_t flags7 = 0, std::uint8_t byte8 = 0, std::uint8_t byte9 = 0,
                                   std::size_t prgBytes = 0, std::size_t chrBytes = 0) {
  std::vector<std::uint8_t> file = {'N', 'E', 'S', 0x1A, prgUnits, chrUnits, flags6, flags7, byte8, byte9, 0, 0, 0, 0, 0, 0};
  if ((flags6 & 0x04U) != 0) file.insert(file.end(), 512, 0xEE);
  if (prgBytes == 0) prgBytes = static_cast<std::size_t>(prgUnits) * 16384;
  if (chrBytes == 0) chrBytes = static_cast<std::size_t>(chrUnits) * 8192;
  for (std::size_t i = 0; i < prgBytes; ++i) file.push_back(static_cast<std::uint8_t>(i * 7 + i / 256));
  for (std::size_t i = 0; i < chrBytes; ++i) file.push_back(static_cast<std::uint8_t>(0x80 + i * 3));
  return file;
}

/** 解析应当失败，且错误码与说明里的片段符合。 */
void expectError(const std::vector<std::uint8_t>& file, std::string_view code, const std::string& fragment = "") {
  try {
    static_cast<void>(parseInes(file));
    FAIL() << "应当抛出异常：" << code;
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), code);
    EXPECT_NE(std::string(error.what()).find(fragment), std::string::npos) << error.what();
  }
}

TEST(Crc32, MatchesTheStandardCheckValueAndCanContinue) {
  const std::string text = "123456789";
  const std::vector<std::uint8_t> bytes(text.begin(), text.end());
  EXPECT_EQ(crc32(bytes), 0xCBF43926U);
  EXPECT_EQ(crc32({}), 0U);
  const std::span<const std::uint8_t> all(bytes);
  EXPECT_EQ(crc32Continue(crc32(all.first(4)), all.subspan(4)), 0xCBF43926U);
}

TEST(InesParser, ParsesAnInes1FileWithMapperMirroringBatteryAndChecksums) {
  const auto file = makeFile(2, 1, 0x13, 0x00);  // mapper 低位 1，垂直镜像，电池
  const auto image = parseInes(file);
  const auto& description = image.description;
  EXPECT_EQ(description.mapper, 1U);
  EXPECT_EQ(description.variant, 0U);
  EXPECT_EQ(description.prgSize, 32768U);
  EXPECT_EQ(description.chrSize, 8192U);
  EXPECT_EQ(description.mirroring, Mirroring::Vertical);
  EXPECT_TRUE(description.battery);
  EXPECT_FALSE(description.trainer);
  ASSERT_EQ(image.prg.size(), 32768U);
  ASSERT_EQ(image.chr.size(), 8192U);
  EXPECT_EQ(image.prg[0], file[16]);
  EXPECT_EQ(image.prg[32767], file[16 + 32767]);
  EXPECT_EQ(image.chr[0], file[16 + 32768]);
  EXPECT_EQ(description.crc32Prg, crc32(image.prg));
  EXPECT_EQ(description.crc32All, crc32(std::span<const std::uint8_t>(file).subspan(16)));
}

TEST(InesParser, CombinesTheMapperNibblesOfTheTwoFlagBytes) {
  EXPECT_EQ(parseInes(makeFile(1, 0, 0x30, 0x00)).description.mapper, 3U);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0x00, 0x10)).description.mapper, 16U);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0xB0, 0xA0)).description.mapper, 171U);
}

TEST(InesParser, TheVariantOfAnInes1HeaderIsByte9ShiftedRightByOne) {
  EXPECT_EQ(parseInes(makeFile(1, 0, 0, 0, 0, 0x00)).description.variant, 0U);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0, 0, 0, 0x01)).description.variant, 0U);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0, 0, 0, 0x02)).description.variant, 1U);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0, 0, 0, 0x05)).description.variant, 2U);
}

TEST(InesParser, ParsesTheHeaderOfTheRealSb2000Bios) {
  // SB2K_BIOS.NES 的文件头：4E 45 53 1A 20 00 B0 A0 00 02 …：32 个 16K 的 PRG，没有 CHR，mapper 171，变体 1。
  std::vector<std::uint8_t> file = {0x4E, 0x45, 0x53, 0x1A, 0x20, 0x00, 0xB0, 0xA0, 0x00, 0x02, 0, 0, 0, 0, 0, 0};
  file.resize(16 + 32U * 16384U, 0x5A);
  const auto image = parseInes(file);
  EXPECT_EQ(image.description.mapper, 171U);
  EXPECT_EQ(image.description.variant, 1U);
  EXPECT_EQ(image.description.prgSize, 524288U);
  EXPECT_EQ(image.description.chrSize, 0U);
  EXPECT_EQ(image.description.mirroring, Mirroring::Horizontal);
  EXPECT_FALSE(image.description.battery);
  EXPECT_TRUE(image.chr.empty());
}

TEST(InesParser, SkipsTheTrainerAndRecordsTheFlag) {
  const auto file = makeFile(1, 1, 0x04);
  const auto image = parseInes(file);
  EXPECT_TRUE(image.description.trainer);
  EXPECT_EQ(image.prg[0], file[16 + 512]);
  EXPECT_EQ(image.prg.size(), 16384U);
}

TEST(InesParser, FourScreenWinsOverTheMirroringBit) {
  EXPECT_EQ(parseInes(makeFile(1, 0, 0x09)).description.mirroring, Mirroring::FourScreen);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0x01)).description.mirroring, Mirroring::Vertical);
  EXPECT_EQ(parseInes(makeFile(1, 0, 0x00)).description.mirroring, Mirroring::Horizontal);
}

TEST(InesParser, IgnoresBytesAfterTheDeclaredContent) {
  auto file = makeFile(1, 1);
  const auto expected = parseInes(file).description.crc32All;
  file.insert(file.end(), 100, 0x77);
  EXPECT_EQ(parseInes(file).description.crc32All, expected);
}

TEST(InesParser, ParsesNes20WithHighMapperBitsSubmapperAndSizeHighNibbles) {
  // flags7 的位 2-3 为 2：NES 2.0。mapper = 0x2B5（低 4 位 5，中间 4 位 B，高 4 位 2），submapper 3，PRG 为 1 个单位。
  const auto image = parseInes(makeFile(1, 1, 0x50, 0xB8, 0x32, 0x00));
  EXPECT_EQ(image.description.mapper, 0x2B5U);
  EXPECT_EQ(image.description.variant, 3U);
  EXPECT_EQ(image.description.prgSize, 16384U);
  EXPECT_EQ(image.description.chrSize, 8192U);
}

TEST(InesParser, Nes20SizeHighNibblesExtendThe8BitCounts) {
  // PRG 的高半字节为 1：共 0x100 + 1 = 257 个 16K 单位；CHR 为 1 个单位。
  const auto image = parseInes(makeFile(1, 1, 0x00, 0x08, 0x00, 0x01, 257U * 16384U));
  EXPECT_EQ(image.description.prgSize, 257U * 16384U);
  EXPECT_EQ(image.description.chrSize, 8192U);
}

TEST(InesParser, Nes20ExponentNotationGivesSizesThatAreNotMultiplesOfTheUnit) {
  // PRG 字节 0x0E 的指数为 3、乘数位为 2：2^3 × (2×2+1) = 40 字节；高半字节必须为 F。CHR 取 0。
  const auto image = parseInes(makeFile(0x0E, 0, 0x00, 0x08, 0x00, 0x0F, 40, 0));
  EXPECT_EQ(image.description.prgSize, 40U);
  EXPECT_EQ(image.description.chrSize, 0U);
}

TEST(InesParser, ANes20MarkerWhoseSizesExceedTheFileFallsBackToInes1) {
  // 位 2-3 为 2，但按 NES 2.0 算出的 PRG 超过文件长度；按旧头解析，mapper 取两个半字节、变体取第 9 字节右移 1 位。
  const auto image = parseInes(makeFile(1, 0, 0x10, 0x88, 0x00, 0x01, 16384, 0));
  EXPECT_EQ(image.description.mapper, 0x81U);
  EXPECT_EQ(image.description.prgSize, 16384U);
  EXPECT_EQ(image.description.variant, 0U);
}

TEST(InesParser, Nes20GivesTheRamSizesAndInes1LeavesThemUnspecified) {
  auto file = makeFile(1, 1, 0x00, 0x08);
  file[10] = 0x71;  // PRG RAM 为 64<<1，PRG NVRAM 为 64<<7
  file[11] = 0x03;  // CHR RAM 为 64<<3，CHR NVRAM 为 0
  const auto& description = parseInes(file).description;
  EXPECT_TRUE(description.nes20);
  EXPECT_EQ(description.prgRamSize, 128U);
  EXPECT_EQ(description.prgNvramSize, 8192U);
  EXPECT_EQ(description.chrRamSize, 512U);
  EXPECT_EQ(description.chrNvramSize, 0U);
  const auto old = parseInes(makeFile(1, 1)).description;
  EXPECT_FALSE(old.nes20);
  EXPECT_FALSE(old.prgRamSize.has_value());
  EXPECT_FALSE(old.prgNvramSize.has_value());
  EXPECT_FALSE(old.chrRamSize.has_value());
  EXPECT_FALSE(old.chrNvramSize.has_value());
}

TEST(InesParserErrors, ReportsAWrongMagicNumber) {
  auto file = makeFile(1, 0);
  file[0] = 'X';
  expectError(file, cart::ERROR_NOT_INES, "NES");
  expectError({}, cart::ERROR_NOT_INES);
  expectError({'N', 'E', 'S', 0x1A, 1, 0}, cart::ERROR_NOT_INES);
}

TEST(InesParserErrors, ReportsUnsupportedFormats) {
  expectError({'U', 'N', 'I', 'F', 0, 0, 0, 0}, cart::ERROR_UNSUPPORTED_FORMAT, "UNIF");
  expectError({'F', 'D', 'S', 0x1A, 1, 0, 0, 0}, cart::ERROR_UNSUPPORTED_FORMAT, "FDS");
}

TEST(InesParserErrors, ReportsATruncatedFileWithTheExpectedSize) {
  auto file = makeFile(2, 1);
  file.resize(file.size() - 1);
  expectError(file, cart::ERROR_TRUNCATED, "被截断");
  file = makeFile(2, 1);
  file.resize(16 + 100);
  expectError(file, cart::ERROR_TRUNCATED, std::to_string(16 + 32768 + 8192));
  // 带训练器时训练器也算在内。
  auto trainer = makeFile(1, 0, 0x04);
  trainer.resize(16 + 300);
  expectError(trainer, cart::ERROR_TRUNCATED, std::to_string(16 + 512 + 16384));
}

TEST(InesParserErrors, ReportsAZeroPrgSize) {
  std::vector<std::uint8_t> file = {'N', 'E', 'S', 0x1A, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  file.resize(16 + 8192, 0);
  expectError(file, cart::ERROR_INVALID_HEADER, "PRG");
}

}  // namespace
