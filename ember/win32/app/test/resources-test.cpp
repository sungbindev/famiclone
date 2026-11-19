/*
 * ------------------------------------------------------------------------
 * 名称：resources-test.cpp
 * 说明：随程序嵌入的资源包的测试：zip 包的读取（条目名称、内容、指纹、不合格的包）、来自资源包的帮助文档站点（只读 help/ 下的条目）、预装插件的释放（只认 plugins/ 下的 .emap、指纹相同时不重复释放、文件缺失或包变化时重新释放、不合格的条目名被跳过）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用测试里现做的 zip，不依赖构建出来的资源包；测试程序本身没有嵌入资源包，所以不测嵌入的取出。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <fstream>

#include "infra/help/archive-help-site.hpp"
#include "infra/plugins/bundled-plugins.hpp"
#include "infra/resources/zip-archive.hpp"
#include "platform/resources/embedded-archive.hpp"
#include "support/temp-directory.hpp"
#include "support/zip-builder.hpp"

namespace ember::win32 {
namespace {

std::string readAll(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::string text(const std::optional<std::vector<std::uint8_t>>& content) { return content ? std::string(content->begin(), content->end()) : std::string("<无>"); }

TEST(ZipArchive, ListsFileNamesAndReadsContent) {
  const auto bytes = test::makeZip({{"help/index.html", "<h1>帮助</h1>"}, {"help/", ""}, {"plugins/a.emap", "A"}});
  const infra::ZipArchive archive(bytes.data(), bytes.size());
  EXPECT_EQ(archive.names(), (std::vector<std::string>{"help/index.html", "plugins/a.emap"}));
  EXPECT_EQ(text(archive.read("help/index.html")), "<h1>帮助</h1>");
  EXPECT_FALSE(archive.read("missing").has_value());
  // 目录条目不是文件
  EXPECT_FALSE(archive.read("help/").has_value());
}

TEST(ZipArchive, FingerprintChangesWithTheContent) {
  const auto a = test::makeZip({{"a", "1"}});
  const auto b = test::makeZip({{"a", "2"}});
  const infra::ZipArchive first(a.data(), a.size());
  const infra::ZipArchive same(a.data(), a.size());
  const infra::ZipArchive other(b.data(), b.size());
  EXPECT_EQ(first.fingerprint(), same.fingerprint());
  EXPECT_NE(first.fingerprint(), other.fingerprint());
}

TEST(ZipArchive, RejectsDataThatIsNotAZip) {
  const std::string notZip = "this is not a zip file";
  EXPECT_THROW(infra::ZipArchive(reinterpret_cast<const unsigned char*>(notZip.data()), notZip.size()), std::runtime_error);
}

TEST(ArchiveHelpSite, ReadsOnlyUnderTheHelpDirectory) {
  const auto bytes = test::makeZip({{"help/index.html", "首页"}, {"help/manual/a.html", "A"}, {"plugins/a.emap", "P"}, {"THIRD_PARTY_NOTICES.txt", "N"}});
  const infra::ZipArchive archive(bytes.data(), bytes.size());
  const infra::ArchiveHelpSite site(archive);
  EXPECT_EQ(text(site.read("index.html")), "首页");
  EXPECT_EQ(text(site.read("manual/a.html")), "A");
  EXPECT_FALSE(site.read("../plugins/a.emap").has_value());
  EXPECT_FALSE(site.read("plugins/a.emap").has_value());
  EXPECT_FALSE(site.read("THIRD_PARTY_NOTICES.txt").has_value());
  EXPECT_FALSE(site.read("missing.html").has_value());
}

TEST(BundledPlugins, ExtractsOnlyEmapFilesDirectlyUnderPlugins) {
  test::TempDirectory directory;
  const auto bytes = test::makeZip({{"plugins/ember-mmc3.emap", "MMC3"}, {"plugins/ember-nrom.emap", "NROM"}, {"plugins/readme.txt", "x"}, {"plugins/sub/inner.emap", "x"}, {"plugins/a..b.emap", "x"}, {"help/index.html", "H"}});
  const infra::ZipArchive archive(bytes.data(), bytes.size());
  const auto target = directory.path() / "bundled-plugins";
  EXPECT_EQ(infra::extractBundledPlugins(archive, target), "");
  EXPECT_EQ(readAll(target / "ember-mmc3.emap"), "MMC3");
  EXPECT_EQ(readAll(target / "ember-nrom.emap"), "NROM");
  EXPECT_TRUE(std::filesystem::exists(target / infra::kBundledPluginsStamp));
  std::size_t count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(target)) {
    static_cast<void>(entry);
    ++count;
  }
  // 两个插件加一个标记文件
  EXPECT_EQ(count, 3U);
  EXPECT_FALSE(std::filesystem::exists(directory.path() / "bundled-plugins.tmp"));
}

TEST(BundledPlugins, SameArchiveIsNotExtractedAgainButMissingFilesAreRestored) {
  test::TempDirectory directory;
  const auto bytes = test::makeZip({{"plugins/a.emap", "A"}});
  const infra::ZipArchive archive(bytes.data(), bytes.size());
  const auto target = directory.path() / "bundled-plugins";
  ASSERT_EQ(infra::extractBundledPlugins(archive, target), "");
  // 用户（或别的程序）在目录里放了文件：指纹相同且文件齐全时保持原样
  { std::ofstream(target / "extra.txt") << "keep"; }
  EXPECT_EQ(infra::extractBundledPlugins(archive, target), "");
  EXPECT_EQ(readAll(target / "extra.txt"), "keep");
  // 文件缺失：重新释放（整个目录换新）
  std::filesystem::remove(target / "a.emap");
  EXPECT_EQ(infra::extractBundledPlugins(archive, target), "");
  EXPECT_EQ(readAll(target / "a.emap"), "A");
  EXPECT_FALSE(std::filesystem::exists(target / "extra.txt"));
}

TEST(BundledPlugins, ChangedArchiveReplacesTheOldPlugins) {
  test::TempDirectory directory;
  const auto target = directory.path() / "bundled-plugins";
  const auto oldBytes = test::makeZip({{"plugins/old.emap", "O"}});
  const auto newBytes = test::makeZip({{"plugins/new.emap", "N"}});
  ASSERT_EQ(infra::extractBundledPlugins(infra::ZipArchive(oldBytes.data(), oldBytes.size()), target), "");
  ASSERT_EQ(infra::extractBundledPlugins(infra::ZipArchive(newBytes.data(), newBytes.size()), target), "");
  EXPECT_TRUE(std::filesystem::exists(target / "new.emap"));
  EXPECT_FALSE(std::filesystem::exists(target / "old.emap"));
}

TEST(EmbeddedArchive, TestExecutableHasNoEmbeddedArchive) { EXPECT_FALSE(platform::embeddedResourceArchive().has_value()); }

}  // namespace
}  // namespace ember::win32
