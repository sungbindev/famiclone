/*
 * ------------------------------------------------------------------------
 * 名称：emap-package-test.cpp
 * 说明：插件包（.emap）读取的测试：合格的包，以及不是 zip、缺 manifest、路径穿越的文件名、目录、符号链接、其他文件、重复文件、超大文件、图标不合格等错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：插件包来自外部，是不可信输入；这里验证每一种不合格的包都以 mappers.invalidPackage 拒绝，并且说明里指出原因。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "../support/plugin-test-support.hpp"
#include "contracts/mappers/mapper-error.hpp"
#include "infra/mappers/emap-package.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::infra::mappers::readEmapPackage;
using ember::test::makeEmap;
using ember::test::PackageFile;

/** 一个合格的清单文本（内容不检查，这里只测包格式）。 */
const std::string MANIFEST = "{}";

/** 只含魔数与 IHDR 的假 PNG：宽、高按参数。 */
std::string makePng(std::uint32_t width, std::uint32_t height) {
  std::string png = {'\x89', 'P', 'N', 'G', '\r', '\n', '\x1A', '\n', 0, 0, 0, 13, 'I', 'H', 'D', 'R'};
  for (const std::uint32_t value : {width, height}) {
    for (int shift = 24; shift >= 0; shift -= 8) png.push_back(static_cast<char>((value >> shift) & 0xFFU));
  }
  png.append(9, '\0');
  return png;
}

/** 读取应当失败，错误码为 mappers.invalidPackage，说明里含有指定片段。 */
void expectInvalid(const std::vector<std::uint8_t>& bytes, const std::string& fragment) {
  try {
    static_cast<void>(readEmapPackage(bytes));
    FAIL() << "应当抛出异常，期望的说明片段：" << fragment;
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), ember::contracts::mappers::ERROR_INVALID_PACKAGE);
    EXPECT_NE(std::string(error.what()).find(fragment), std::string::npos) << error.what();
  }
}

/** 把第 index 个条目的中央目录记录改成 Unix 符号链接。 */
void markAsSymlink(std::vector<std::uint8_t>& zip, int index) {
  int seen = 0;
  for (std::size_t position = 0; position + 46 <= zip.size(); ++position) {
    if (zip[position] != 'P' || zip[position + 1] != 'K' || zip[position + 2] != 0x01 || zip[position + 3] != 0x02) continue;
    if (seen++ != index) continue;
    zip[position + 5] = 3;  // 创建者为 Unix
    zip[position + 38] = 0;
    zip[position + 39] = 0;
    zip[position + 40] = 0xFF;
    zip[position + 41] = 0xA1;  // 外部属性高 16 位：0xA1FF，符号链接
    return;
  }
  FAIL() << "没有找到第 " << index << " 个中央目录记录";
}

TEST(EmapPackage, ReadsTheManifestTheScriptAndTheIcon) {
  const auto package = readEmapPackage(makeEmap({{"manifest.json", MANIFEST}, {"main.js", "export default {};"}, {"icon.png", makePng(64, 64)}}));
  EXPECT_EQ(package.manifestText, MANIFEST);
  EXPECT_EQ(package.scriptText, "export default {};");
  EXPECT_TRUE(package.hasScript);
  EXPECT_TRUE(package.hasIcon);
}

TEST(EmapPackage, AManifestAloneIsEnough) {
  const auto package = readEmapPackage(makeEmap({{"manifest.json", MANIFEST}}));
  EXPECT_FALSE(package.hasScript);
  EXPECT_FALSE(package.hasIcon);
  EXPECT_TRUE(package.scriptText.empty());
}

TEST(EmapPackage, ReadsTheOfficialNromPackage) {
  const auto package = readEmapPackage(ember::test::officialPluginPackage("nrom"));
  EXPECT_NE(package.manifestText.find("\"id\": \"nrom\""), std::string::npos);
  EXPECT_TRUE(package.hasScript);
  EXPECT_NE(package.scriptText.find("onKey"), std::string::npos);
}

TEST(EmapPackageErrors, RejectsAFileThatIsNotAZip) {
  expectInvalid({}, "空");
  expectInvalid(std::vector<std::uint8_t>(100, 0x41), "不是有效的 zip");
  auto truncated = makeEmap({{"manifest.json", MANIFEST}});
  truncated.resize(truncated.size() / 2);
  expectInvalid(truncated, "不是有效的 zip");
}

TEST(EmapPackageErrors, RejectsAPackageOverTheSizeLimit) {
  expectInvalid(std::vector<std::uint8_t>(ember::infra::mappers::MAX_PACKAGE_BYTES + 1, 0), "上限");
}

TEST(EmapPackageErrors, RejectsAZipWithoutEntries) { expectInvalid(makeEmap({}), "没有文件"); }

TEST(EmapPackageErrors, RejectsAMissingManifest) { expectInvalid(makeEmap({{"main.js", "export default {};"}}), "缺少 manifest.json"); }

TEST(EmapPackageErrors, RejectsPathTraversalAndUnsafeNames) {
  for (const std::string name : {"../manifest.json", "a/../../main.js", "dir\\manifest.json", "C:manifest.json", ".."}) {
    expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {name, "x"}}), "文件名不安全");
  }
}

TEST(EmapPackageErrors, RejectsAnAbsolutePathName) {
  // zip 写入端不允许以 / 开头的名字，所以先写成 Xmain.js，再把本地文件头与中央目录里的 X 改成 /。
  auto zip = makeEmap({{"manifest.json", MANIFEST}, {"Xmain.js", "x"}});
  int replaced = 0;
  for (std::size_t position = 0; position + 8 <= zip.size(); ++position) {
    if (std::string(zip.begin() + static_cast<std::ptrdiff_t>(position), zip.begin() + static_cast<std::ptrdiff_t>(position) + 8) == "Xmain.js") {
      zip[position] = '/';
      ++replaced;
    }
  }
  ASSERT_EQ(replaced, 2);
  expectInvalid(zip, "文件名不安全");
}

TEST(EmapPackageErrors, RejectsDirectoriesAndOtherFiles) {
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"sub/", ""}}), "目录");
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"sub/main.js", "x"}}), "其他文件");
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"readme.txt", "x"}}), "readme.txt");
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"MAIN.JS", "x"}}), "其他文件");
}

TEST(EmapPackageErrors, RejectsDuplicateEntries) { expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"manifest.json", MANIFEST}}), "重复"); }

TEST(EmapPackageErrors, RejectsTooManyEntries) {
  std::vector<PackageFile> files = {{"manifest.json", MANIFEST}};
  for (int index = 0; index < 20; ++index) files.emplace_back("file" + std::to_string(index) + ".txt", "x");
  expectInvalid(makeEmap(files), "文件过多");
}

TEST(EmapPackageErrors, RejectsSymbolicLinks) {
  auto zip = makeEmap({{"manifest.json", MANIFEST}, {"main.js", "export default {};"}});
  markAsSymlink(zip, 1);
  expectInvalid(zip, "符号链接");
}

TEST(EmapPackageErrors, RejectsFilesOverTheirSizeLimits) {
  // 内容全是空格，压缩后很小，但解压后超过上限：按中央目录声明的大小拒绝，不会真的解压。
  expectInvalid(makeEmap({{"manifest.json", std::string(ember::infra::mappers::MAX_MANIFEST_BYTES + 1, ' ')}}), "manifest.json 解压后超过大小上限");
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"main.js", std::string(ember::infra::mappers::MAX_SCRIPT_BYTES + 1, ' ')}}), "main.js 解压后超过大小上限");
}

TEST(EmapPackageErrors, RejectsABadIcon) {
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"icon.png", "not a png at all, just text"}}), "不是 PNG");
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"icon.png", makePng(64, 32)}}), "正方形");
  expectInvalid(makeEmap({{"manifest.json", MANIFEST}, {"icon.png", makePng(32, 32)}}), "不小于 64");
}

TEST(EmapPackageErrors, RejectsACorruptedEntry) {
  auto zip = makeEmap({{"manifest.json", std::string(2000, 'a')}});
  // 破坏压缩数据：本地文件头之后的内容。
  for (std::size_t position = 40; position < 50 && position < zip.size(); ++position) zip[position] ^= 0xFF;
  expectInvalid(zip, "损坏");
}

}  // namespace
