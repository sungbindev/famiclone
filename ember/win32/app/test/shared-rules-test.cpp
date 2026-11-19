/*
 * ------------------------------------------------------------------------
 * 名称：shared-rules-test.cpp
 * 说明：共用规则的测试：最近打开的卡带清单、界面状态、记住上次目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则与文件格式和 Electron 外壳的对应测试一致（两个外壳共用同一份用户数据）。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "shared/last-directories/last-directories.hpp"
#include "shared/preferences/allow-multiple-instances.hpp"
#include "shared/recent-cartridges/recent-cartridges.hpp"
#include "shared/shell-state/shell-state.hpp"
#include "shared/window/window-shadow-image.hpp"

namespace {

using nlohmann::json;
using namespace ember::win32::shared;

TEST(RecentCartridges, AddMovesToFrontAndKeepsAtMostTen) {
  std::vector<std::string> list;
  for (int i = 0; i < 12; ++i) list = addRecentCartridge(list, "D:\\roms\\" + std::to_string(i) + ".nes");
  ASSERT_EQ(list.size(), kMaxRecentCartridges);
  EXPECT_EQ(list.front(), "D:\\roms\\11.nes");
  EXPECT_EQ(list.back(), "D:\\roms\\2.nes");
  list = addRecentCartridge(list, "D:\\roms\\5.nes");
  EXPECT_EQ(list.front(), "D:\\roms\\5.nes");
  EXPECT_EQ(list.size(), kMaxRecentCartridges);
}

TEST(RecentCartridges, AddIgnoresNonNesPaths) {
  const std::vector<std::string> list{"a.nes"};
  EXPECT_EQ(addRecentCartridge(list, "b.zip"), list);
  EXPECT_EQ(addRecentCartridge(list, ""), list);
  EXPECT_EQ(addRecentCartridge(list, "C.NES").front(), "C.NES");
}

TEST(RecentCartridges, RemoveTakesOutOnlyThatPath) {
  EXPECT_EQ(removeRecentCartridge({"a.nes", "b.nes"}, "a.nes"), std::vector<std::string>{"b.nes"});
  const std::vector<std::string> list{"a.nes"};
  EXPECT_EQ(removeRecentCartridge(list, "x.nes"), list);
}

TEST(RecentCartridges, ParseKeepsValidDistinctItemsAndSurvivesGarbage) {
  const json file = {{"version", 1}, {"items", {{{"path", "a.nes"}}, {{"path", "a.nes"}}, {{"path", "b.zip"}}, {{"path", 5}}, 7, {{"path", "c.NES"}}}}};
  EXPECT_EQ(parseRecentCartridges(file), (std::vector<std::string>{"a.nes", "c.NES"}));
  EXPECT_TRUE(parseRecentCartridges(json(nullptr)).empty());
  EXPECT_TRUE(parseRecentCartridges(json::array()).empty());
  EXPECT_TRUE(parseRecentCartridges({{"items", "x"}}).empty());
}

TEST(RecentCartridges, FileFormatRoundTripsAndNameIsTheFileName) {
  const std::vector<std::string> list{"D:\\roms\\mario.nes", "C:/a/b.nes"};
  EXPECT_EQ(parseRecentCartridges(toRecentCartridgesFile(list)), list);
  EXPECT_EQ(toRecentCartridgesFile(list)["version"], 1);
  EXPECT_EQ(recentCartridgeName("D:\\roms\\mario.nes"), "mario.nes");
  EXPECT_EQ(recentCartridgeName("C:/a/b.nes"), "b.nes");
  EXPECT_EQ(recentCartridgeName("plain.nes"), "plain.nes");
}

TEST(ShellState, DefaultsWhenMissingOrBroken) {
  for (const json& raw : {json(nullptr), json::array(), json("x"), json(5)}) {
    const ShellState state = parseShellState(raw);
    EXPECT_FALSE(state.window);
    EXPECT_FALSE(state.layout.selectedMachine);
    EXPECT_TRUE(state.layout.autoCollapseOffPanels.empty());
    EXPECT_FALSE(state.releaseNotesVersion);
  }
}

TEST(ShellState, ReadsEachValidItemAndReplacesOnlyTheBrokenOnes) {
  const ShellState state = parseShellState({{"window", {{"x", 10}, {"y", -20}}},
                                            {"layout", {{"selectedMachine", "sb2000"}, {"autoCollapseOffPanels", {"runtime.panel", "bad id"}}}},
                                            {"releaseNotesVersion", "0.2.2"}});
  ASSERT_TRUE(state.window);
  EXPECT_EQ(*state.window, (WindowPlacement{10, -20}));
  EXPECT_EQ(state.layout.selectedMachine, "sb2000");
  // 清单里有一项不合格，整个清单不用
  EXPECT_TRUE(state.layout.autoCollapseOffPanels.empty());
  EXPECT_EQ(state.releaseNotesVersion, "0.2.2");
}

TEST(ShellState, RejectsOutOfRangeWindowAndMalformedFields) {
  EXPECT_FALSE(parseShellState({{"window", {{"x", 100001}, {"y", 0}}}}).window);
  EXPECT_FALSE(parseShellState({{"window", {{"x", 1.5}, {"y", 0}}}}).window);
  EXPECT_FALSE(parseShellState({{"window", {{"x", 1}}}}).window);
  EXPECT_FALSE(parseShellState({{"layout", {{"selectedMachine", "Bad Machine"}}}}).layout.selectedMachine);
  EXPECT_FALSE(parseShellState({{"releaseNotesVersion", "v1"}}).releaseNotesVersion);
  const auto dup = parseShellState({{"layout", {{"autoCollapseOffPanels", {"runtime.panel", "runtime.panel"}}}}});
  EXPECT_TRUE(dup.layout.autoCollapseOffPanels.empty());
  const auto ok = parseShellState({{"layout", {{"autoCollapseOffPanels", {"runtime.panel", "media.devices"}}}}});
  EXPECT_EQ(ok.layout.autoCollapseOffPanels.size(), 2u);
}

TEST(ShellState, SerialisesToTheSameFormat) {
  ShellState state;
  state.window = WindowPlacement{5, 6};
  state.layout.selectedMachine = "fc";
  state.layout.autoCollapseOffPanels = {"runtime.panel"};
  state.releaseNotesVersion = "1.0.0";
  const json written = toJson(state);
  EXPECT_EQ(written["window"]["x"], 5);
  EXPECT_EQ(written["layout"]["selectedMachine"], "fc");
  const ShellState back = parseShellState(written);
  EXPECT_EQ(back.window, state.window);
  EXPECT_EQ(back.layout, state.layout);
  EXPECT_EQ(back.releaseNotesVersion, state.releaseNotesVersion);
  EXPECT_TRUE(toJson(ShellState{})["window"].is_null());
}

TEST(AllowMultipleInstances, OnlyAnExplicitTrueInAVersionOneFileCounts) {
  EXPECT_TRUE(readAllowMultipleInstances({{"version", 1}, {"values", {{"app.allowMultipleInstances", true}}}}));
  EXPECT_FALSE(readAllowMultipleInstances({{"version", 1}, {"values", {{"app.allowMultipleInstances", false}}}}));
  EXPECT_FALSE(readAllowMultipleInstances({{"version", 1}, {"values", {{"app.allowMultipleInstances", "true"}}}}));
  EXPECT_FALSE(readAllowMultipleInstances({{"version", 2}, {"values", {{"app.allowMultipleInstances", true}}}}));
  EXPECT_FALSE(readAllowMultipleInstances({{"version", 1}, {"values", {{"other", true}}}}));
  EXPECT_FALSE(readAllowMultipleInstances({{"version", 1}}));
  EXPECT_FALSE(readAllowMultipleInstances(json(nullptr)));
  EXPECT_FALSE(readAllowMultipleInstances(json::array()));
}

TEST(LastDirectories, PurposeAndDirectoryRules) {
  EXPECT_TRUE(isValidDirectoryPurpose("cartridge"));
  EXPECT_TRUE(isValidDirectoryPurpose("tapeRecording2"));
  EXPECT_FALSE(isValidDirectoryPurpose("1abc"));
  EXPECT_FALSE(isValidDirectoryPurpose("a-b"));
  EXPECT_FALSE(isValidDirectoryPurpose(""));
  EXPECT_FALSE(isValidDirectoryPurpose(std::string(65, 'a')));
  EXPECT_TRUE(isValidRememberedDirectory("D:\\roms"));
  EXPECT_FALSE(isValidRememberedDirectory(""));
  EXPECT_FALSE(isValidRememberedDirectory(std::string(1025, 'x')));
}

TEST(LastDirectories, ParseSkipsBadItemsAndChecksTheVersion) {
  const auto items = parseLastDirectories({{"version", 1}, {"directories", {{"cartridge", "D:\\roms"}, {"bad-purpose", "x"}, {"floppy", ""}, {"tape", 5}}}});
  EXPECT_EQ(items, (std::map<std::string, std::string>{{"cartridge", "D:\\roms"}}));
  EXPECT_TRUE(parseLastDirectories({{"version", 2}, {"directories", {{"a", "b"}}}}).empty());
  EXPECT_TRUE(parseLastDirectories(json(nullptr)).empty());
  EXPECT_EQ(parseLastDirectories(toLastDirectoriesFile({{"bios", "C:\\b"}})), (std::map<std::string, std::string>{{"bios", "C:\\b"}}));
}

/** 阴影图像在 (x, y) 处的透明度。 */
unsigned shadowAlpha(const WindowShadowImage& image, int x, int y) {
  return image.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(x)] >> 24;
}

TEST(WindowShadowImage, IsTheWindowPlusAMarginOnEverySide) {
  const auto image = renderWindowShadow(520, 480, 1.0, true);
  EXPECT_EQ(image.margin, shadowMargin(1.0));
  EXPECT_EQ(image.width, 520 + 2 * image.margin);
  EXPECT_EQ(image.height, 480 + 2 * image.margin);
  EXPECT_EQ(image.pixels.size(), static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height));
  EXPECT_EQ(shadowMargin(1.5), 54);
}

TEST(WindowShadowImage, IsEmptyForANonPositiveSize) {
  EXPECT_TRUE(renderWindowShadow(0, 480, 1.0, true).pixels.empty());
  EXPECT_TRUE(renderWindowShadow(520, -1, 1.0, true).pixels.empty());
  EXPECT_TRUE(renderWindowShadow(520, 480, 0, true).pixels.empty());
}

TEST(WindowShadowImage, FadesFromTheWindowEdgeToNothingAtTheFarEdge) {
  const auto image = renderWindowShadow(520, 480, 1.0, true);
  const int middleY = image.height / 2;
  const int edge = image.margin - 1;
  // 紧贴窗口的一圈有明显的阴影，越远越淡，最外一圈几乎透明
  EXPECT_GT(shadowAlpha(image, edge, middleY), 40u);
  EXPECT_LT(shadowAlpha(image, edge - 10, middleY), shadowAlpha(image, edge, middleY));
  EXPECT_LE(shadowAlpha(image, 0, middleY), 2u);
  EXPECT_LE(shadowAlpha(image, image.width / 2, image.height - 1), 2u);
  // 窗口里面（被窗口盖住）最深
  EXPECT_GE(shadowAlpha(image, image.width / 2, image.height / 2), shadowAlpha(image, edge, middleY));
}

TEST(WindowShadowImage, ShadowIsShiftedDownAndLeftRightSymmetric) {
  const auto image = renderWindowShadow(520, 480, 1.0, true);
  const int middleX = image.width / 2;
  const int middleY = image.height / 2;
  EXPECT_GT(shadowAlpha(image, middleX, image.height - image.margin + 2), shadowAlpha(image, middleX, image.margin - 3));
  for (int offset = 1; offset < image.margin; ++offset) EXPECT_EQ(shadowAlpha(image, offset, middleY), shadowAlpha(image, image.width - 1 - offset, middleY));
}

TEST(WindowShadowImage, InactiveWindowsHaveALighterShadow) {
  const auto active = renderWindowShadow(520, 480, 1.0, true);
  const auto inactive = renderWindowShadow(520, 480, 1.0, false);
  const int x = active.margin - 1;
  const int y = active.height / 2;
  EXPECT_LT(shadowAlpha(inactive, x, y), shadowAlpha(active, x, y));
  EXPECT_GT(shadowAlpha(inactive, x, y), 0u);
}

}  // namespace
