/*
 * ------------------------------------------------------------------------
 * 名称：launch-args-test.cpp
 * 说明：启动参数解析的测试：卡带路径、调试端口与令牌、用户数据目录的取值规则，不合格参数的忽略。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "shared/launch/launch-args.hpp"

namespace {

using ember::win32::shared::parseLaunchArguments;
using ember::win32::shared::UserDataPlace;

TEST(LaunchArguments, ReadsCartridgePathCaseInsensitively) {
  EXPECT_EQ(parseLaunchArguments({"--cartridge=D:\\roms\\game.nes"}).cartridgePath, "D:\\roms\\game.nes");
  EXPECT_EQ(parseLaunchArguments({"--cartridge=D:\\roms\\GAME.NES"}).cartridgePath, "D:\\roms\\GAME.NES");
}

TEST(LaunchArguments, IgnoresCartridgeThatIsNotNes) {
  EXPECT_FALSE(parseLaunchArguments({"--cartridge=D:\\roms\\game.zip"}).cartridgePath);
  EXPECT_FALSE(parseLaunchArguments({"--cartridge="}).cartridgePath);
  EXPECT_FALSE(parseLaunchArguments({"--cartridge"}).cartridgePath);
}

TEST(LaunchArguments, OnlyTheFirstCartridgeArgumentCounts) {
  EXPECT_FALSE(parseLaunchArguments({"--cartridge=a.zip", "--cartridge=b.nes"}).cartridgePath);
}

TEST(LaunchArguments, ReadsDebugPortAndTokenInBothForms) {
  const auto equals = parseLaunchArguments({"--debug-port=5688", "--debug-token=secret"});
  EXPECT_EQ(equals.debugPort, 5688);
  EXPECT_EQ(equals.debugToken, "secret");
  const auto spaced = parseLaunchArguments({"--debug-port", "5689", "--debug-token", "abc"});
  EXPECT_EQ(spaced.debugPort, 5689);
  EXPECT_EQ(spaced.debugToken, "abc");
}

TEST(LaunchArguments, RejectsInvalidPorts) {
  for (const char* value : {"0", "65536", "-1", "abc", "123456", "", "12.5"}) {
    EXPECT_FALSE(parseLaunchArguments({std::string("--debug-port=") + value}).debugPort) << value;
  }
  EXPECT_EQ(parseLaunchArguments({"--debug-port=1"}).debugPort, 1);
  EXPECT_EQ(parseLaunchArguments({"--debug-port=65535"}).debugPort, 65535);
  EXPECT_FALSE(parseLaunchArguments({"--debug-port"}).debugPort);
}

TEST(LaunchArguments, RejectsInvalidTokens) {
  EXPECT_FALSE(parseLaunchArguments({"--debug-token="}).debugToken);
  EXPECT_FALSE(parseLaunchArguments({"--debug-token=a b"}).debugToken);
  EXPECT_FALSE(parseLaunchArguments({"--debug-token=\xE4\xB8\xAD"}).debugToken);
  EXPECT_FALSE(parseLaunchArguments({"--debug-token=" + std::string(129, 'x')}).debugToken);
  EXPECT_EQ(parseLaunchArguments({"--debug-token=" + std::string(128, 'x')}).debugToken, std::string(128, 'x'));
}

TEST(LaunchArguments, ReadsUserDataDirectory) {
  EXPECT_EQ(parseLaunchArguments({"--user-data-dir=D:\\data\\ember"}).userDataDirectory, "D:\\data\\ember");
  EXPECT_EQ(parseLaunchArguments({"--user-data-dir", "D:\\data\\ember"}).userDataDirectory, "D:\\data\\ember");
  EXPECT_FALSE(parseLaunchArguments({"--user-data-dir="}).userDataDirectory);
}

TEST(LaunchArguments, ReadsMoveUserDataAndWaitProcessId) {
  const auto both = parseLaunchArguments({"--move-user-data=executable", "--wait-pid=1234"});
  EXPECT_EQ(both.moveUserData, UserDataPlace::ExecutableDirectory);
  EXPECT_EQ(both.waitProcessId, 1234u);
  EXPECT_EQ(parseLaunchArguments({"--move-user-data", "profile"}).moveUserData, UserDataPlace::UserProfile);
  EXPECT_FALSE(parseLaunchArguments({"--move-user-data=elsewhere"}).moveUserData);
  EXPECT_FALSE(parseLaunchArguments({"--wait-pid=0"}).waitProcessId);
  EXPECT_FALSE(parseLaunchArguments({"--wait-pid=12x"}).waitProcessId);
  EXPECT_FALSE(parseLaunchArguments({"--wait-pid=99999999999"}).waitProcessId);
}

TEST(LaunchArguments, ReadsApplyUpdateTarget) {
  EXPECT_EQ(parseLaunchArguments({"--apply-update=D:\\Apps\\Ember.exe"}).applyUpdateTarget, "D:\\Apps\\Ember.exe");
  EXPECT_EQ(parseLaunchArguments({"--apply-update", "D:\\Apps\\Ember.exe"}).applyUpdateTarget, "D:\\Apps\\Ember.exe");
  EXPECT_FALSE(parseLaunchArguments({"--apply-update="}).applyUpdateTarget);
}

TEST(LaunchArguments, EmptyArgumentsGiveNothing) {
  const auto result = parseLaunchArguments({});
  EXPECT_FALSE(result.cartridgePath);
  EXPECT_FALSE(result.debugPort);
  EXPECT_FALSE(result.debugToken);
  EXPECT_FALSE(result.userDataDirectory);
  EXPECT_FALSE(result.moveUserData);
  EXPECT_FALSE(result.waitProcessId);
  EXPECT_FALSE(result.applyUpdateTarget);
}

}  // namespace
