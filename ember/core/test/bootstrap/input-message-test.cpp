/*
 * ------------------------------------------------------------------------
 * 名称：input-message-test.cpp
 * 说明：输入消息解析的测试：键盘、鼠标、游戏手柄三种消息的字段与范围，以及各种不合格的消息。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <string>
#include <variant>

#include "bootstrap/input-message.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::bootstrap::parseInputMessage;
namespace hw = ember::contracts::hardware;

TEST(InputMessage, ParsesAKeyEvent) {
  const auto event = parseInputMessage(R"({"type":"key","code":"Numpad1","down":true})");
  const auto* key = std::get_if<hw::KeyInput>(&event);
  ASSERT_NE(key, nullptr);
  EXPECT_EQ(key->code, "Numpad1");
  EXPECT_TRUE(key->down);
}

TEST(InputMessage, ParsesAMouseEvent) {
  const auto event = parseInputMessage(R"({"type":"mouse","kind":"wheel","x":10.5,"y":-3,"dx":1,"dy":0,"buttons":5,"wheel":-2})");
  const auto* mouse = std::get_if<hw::MouseInput>(&event);
  ASSERT_NE(mouse, nullptr);
  EXPECT_EQ(mouse->kind, hw::MouseKind::Wheel);
  EXPECT_DOUBLE_EQ(mouse->x, 10.5);
  EXPECT_DOUBLE_EQ(mouse->y, -3);
  EXPECT_DOUBLE_EQ(mouse->dx, 1);
  EXPECT_EQ(mouse->buttons, 5);
  EXPECT_DOUBLE_EQ(mouse->wheel, -2);
}

TEST(InputMessage, ParsesEveryMouseKind) {
  const std::pair<const char*, hw::MouseKind> kinds[] = {{"move", hw::MouseKind::Move}, {"down", hw::MouseKind::Down}, {"up", hw::MouseKind::Up}, {"wheel", hw::MouseKind::Wheel}, {"leave", hw::MouseKind::Leave}};
  for (const auto& [name, kind] : kinds) {
    const auto event = parseInputMessage(std::string(R"({"type":"mouse","kind":")") + name + R"(","x":0,"y":0,"dx":0,"dy":0,"buttons":0,"wheel":0})");
    EXPECT_EQ(std::get<hw::MouseInput>(event).kind, kind) << name;
  }
}

TEST(InputMessage, ParsesAGamepadEvent) {
  const auto event = parseInputMessage(R"({"type":"gamepad","index":2,"id":"Demo Pad [STANDARD GAMEPAD]","connected":true,"buttons":[true,false,true],"axes":[0.5,-1]})");
  const auto* pad = std::get_if<hw::GamepadInput>(&event);
  ASSERT_NE(pad, nullptr);
  EXPECT_EQ(pad->index, 2);
  EXPECT_EQ(pad->id, "Demo Pad [STANDARD GAMEPAD]");
  EXPECT_TRUE(pad->connected);
  EXPECT_EQ(pad->buttons, (std::vector<bool>{true, false, true}));
  EXPECT_EQ(pad->axes, (std::vector<double>{0.5, -1}));
}

TEST(InputMessage, RejectsMessagesThatAreNotWellFormed) {
  const char* bad[] = {
      "",
      "垃圾",
      "[]",
      "42",
      R"({})",
      R"({"type":"buttons","port":0,"buttons":1})",
      R"({"type":"key","code":"KeyW"})",
      R"({"type":"key","code":"KeyW","down":1})",
      R"({"type":"key","code":"","down":true})",
      R"({"type":"key","code":"Key W","down":true})",
      R"({"type":"key","code":"KeyW","down":true,"extra":1})",
      R"({"type":"mouse","kind":"drag","x":0,"y":0,"dx":0,"dy":0,"buttons":0,"wheel":0})",
      R"({"type":"mouse","kind":"move","x":2000000,"y":0,"dx":0,"dy":0,"buttons":0,"wheel":0})",
      R"({"type":"mouse","kind":"move","x":0,"y":0,"dx":0,"dy":0,"buttons":8,"wheel":0})",
      R"({"type":"mouse","kind":"move","x":0,"y":0,"dx":0,"dy":0,"buttons":1.5,"wheel":0})",
      R"({"type":"mouse","kind":"move","x":0,"y":0,"dx":0,"dy":0,"buttons":0})",
      R"({"type":"gamepad","index":16,"id":"","connected":true,"buttons":[],"axes":[]})",
      R"({"type":"gamepad","index":0,"connected":true,"buttons":[],"axes":[]})",
      R"({"type":"gamepad","index":0,"id":5,"connected":true,"buttons":[],"axes":[]})",
      R"({"type":"gamepad","index":0,"id":"","connected":1,"buttons":[],"axes":[]})",
      R"({"type":"gamepad","index":0,"id":"","connected":true,"buttons":[1],"axes":[]})",
      R"({"type":"gamepad","index":0,"id":"","connected":true,"buttons":[],"axes":[2]})",
      R"({"type":"gamepad","index":0,"id":"","connected":true,"buttons":[],"axes":["0"]})",
  };
  for (const char* text : bad) {
    try {
      static_cast<void>(parseInputMessage(text));
      ADD_FAILURE() << "应当拒绝：" << text;
    } catch (const ember::shared::CoreError& error) {
      EXPECT_EQ(error.code(), ember::shared::ERROR_INVALID_PARAMS) << text;
    }
  }
}

TEST(InputMessage, RejectsTooManyGamepadButtonsAndAxes) {
  std::string buttons = "[";
  for (int index = 0; index < 33; ++index) buttons += index == 0 ? "true" : ",true";
  EXPECT_THROW(parseInputMessage(R"({"type":"gamepad","index":0,"id":"","connected":true,"buttons":)" + buttons + R"(],"axes":[]})"), ember::shared::CoreError);
  std::string axes = "[";
  for (int index = 0; index < 17; ++index) axes += index == 0 ? "0" : ",0";
  EXPECT_THROW(parseInputMessage(R"({"type":"gamepad","index":0,"id":"","connected":true,"buttons":[],"axes":)" + axes + "]}"), ember::shared::CoreError);
  EXPECT_THROW(parseInputMessage(R"({"type":"gamepad","index":0,"id":")" + std::string(257, 'x') + R"(","connected":true,"buttons":[],"axes":[]})"), ember::shared::CoreError);
}

}  // namespace
