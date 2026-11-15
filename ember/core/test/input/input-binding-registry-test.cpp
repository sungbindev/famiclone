/*
 * ------------------------------------------------------------------------
 * 名称：input-binding-registry-test.cpp
 * 说明：手柄绑定服务的测试：出厂默认、校验规则、变化事件、保存与读回、文件不合格时的处理。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/input/input-error.hpp"
#include "infra/input/input-binding-registry.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::infra::input::InputBindingRegistry;
using ember::infra::input::InputBindingRegistryOptions;
namespace api = ember::contracts::input;

/** 带事件记录与错误日志记录的绑定服务。 */
struct Rig {
  explicit Rig(const std::filesystem::path& file = {}) : events(std::make_shared<ember::test::RecordingEventSink>()), errors(std::make_shared<ember::test::RecordingErrorLog>()) {
    InputBindingRegistryOptions options;
    options.file = file;
    options.events = events;
    options.errors = errors;
    registry = std::make_unique<InputBindingRegistry>(std::move(options));
  }

  std::shared_ptr<ember::test::RecordingEventSink> events;
  std::shared_ptr<ember::test::RecordingErrorLog> errors;
  std::unique_ptr<InputBindingRegistry> registry;
};

std::string codeOf(const std::function<void()>& call) {
  try {
    call();
  } catch (const ember::shared::CoreError& error) {
    return std::string(error.code());
  }
  return {};
}

api::PadGamepadAssignment assignment(const std::string& id, std::int64_t occurrence = 0) {
  api::PadGamepadAssignment pad;
  pad.id = id;
  pad.occurrence = occurrence;
  pad.bindings = {{"a", "button", 1, std::nullopt}, {"left", "axis", 0, std::string("negative")}};
  return pad;
}

TEST(InputBindingRegistry, StartsWithTheFactoryDefaults) {
  Rig rig;
  const auto slots = *rig.registry->bindings();
  ASSERT_EQ(slots.size(), 2U);
  EXPECT_EQ(slots[0].keys.size(), 10U) << "玩家 1 的 10 个键都绑定到键盘";
  EXPECT_TRUE(slots[1].keys.empty()) << "玩家 2 默认没有绑定";
  EXPECT_FALSE(slots[0].gamepad.has_value());
  const auto find = [&](const std::string& button) {
    for (const auto& key : slots[0].keys) {
      if (key.button == button) return key.code;
    }
    return std::string();
  };
  EXPECT_EQ(find("a"), "KeyX");
  EXPECT_EQ(find("b"), "KeyZ");
  EXPECT_EQ(find("turboA"), "KeyV");
  EXPECT_EQ(find("turboB"), "KeyC");
  EXPECT_EQ(find("start"), "Enter");
  EXPECT_EQ(find("select"), "ShiftRight");
}

TEST(InputBindingRegistry, SetReplacesTheBindingsAndPublishesAnEventOnlyWhenTheyChange) {
  Rig rig;
  auto slots = *rig.registry->bindings();
  slots[1].gamepad = assignment("Demo Pad");
  const auto result = rig.registry->set(slots);
  EXPECT_EQ(result->at(1).gamepad->id, "Demo Pad");
  ASSERT_EQ(rig.events->events.size(), 1U);
  EXPECT_EQ(rig.events->events[0].first, "input.bindingsChanged");
  EXPECT_EQ(rig.events->events[0].second["slots"][1]["gamepad"]["id"], "Demo Pad");
  rig.registry->set(slots);
  EXPECT_EQ(rig.events->events.size(), 1U) << "内容没有变化时不发事件";
  rig.registry->reset();
  EXPECT_EQ(rig.events->events.size(), 2U);
  EXPECT_FALSE(rig.registry->bindings()->at(1).gamepad.has_value());
  rig.registry->reset();
  EXPECT_EQ(rig.events->events.size(), 2U);
}

TEST(InputBindingRegistry, RejectsBindingsThatDoNotMakeSense) {
  Rig rig;
  const auto good = *rig.registry->bindings();
  const auto rejects = [&](const std::function<void(api::PadSlots&)>& change) {
    auto slots = good;
    change(slots);
    return codeOf([&] { rig.registry->set(slots); });
  };
  EXPECT_EQ(rejects([](api::PadSlots& slots) { slots.pop_back(); }), api::ERROR_INVALID_BINDINGS);
  EXPECT_EQ(rejects([](api::PadSlots& slots) { slots[0].keys.push_back({"a", "KeyQ"}); }), api::ERROR_INVALID_BINDINGS) << "同一个手柄键绑定了两次";
  EXPECT_EQ(rejects([](api::PadSlots& slots) { slots[1].keys.push_back({"a", "KeyX"}); }), api::ERROR_INVALID_BINDINGS) << "同一个物理键绑定给了两个玩家";
  EXPECT_EQ(rejects([](api::PadSlots& slots) { slots[1].keys.push_back({"jump", "KeyQ"}); }), api::ERROR_INVALID_BINDINGS) << "不认识的手柄键";
  EXPECT_EQ(rejects([](api::PadSlots& slots) { slots[1].keys.push_back({"a", "Key Q"}); }), api::ERROR_INVALID_BINDINGS) << "物理键位只能是字母与数字";
  EXPECT_EQ(rejects([](api::PadSlots& slots) { slots[1].gamepad = assignment(""); }), api::ERROR_INVALID_BINDINGS);
  EXPECT_EQ(rejects([](api::PadSlots& slots) {
              slots[0].gamepad = assignment("Pad");
              slots[1].gamepad = assignment("Pad");
            }),
            api::ERROR_INVALID_BINDINGS)
      << "同一个手柄分给了两个玩家";
  EXPECT_EQ(rejects([](api::PadSlots& slots) {
              slots[0].gamepad = assignment("Pad");
              slots[0].gamepad->bindings.push_back({"a", "button", 2, std::nullopt});
            }),
            api::ERROR_INVALID_BINDINGS)
      << "实体手柄上同一个手柄键绑定了两次";
  EXPECT_EQ(rejects([](api::PadSlots& slots) {
              slots[0].gamepad = assignment("Pad");
              slots[0].gamepad->bindings = {{"up", "axis", 1, std::nullopt}};
            }),
            api::ERROR_INVALID_BINDINGS)
      << "轴必须写方向";
  EXPECT_EQ(rejects([](api::PadSlots& slots) {
              slots[0].gamepad = assignment("Pad");
              slots[0].gamepad->bindings = {{"up", "button", 40, std::nullopt}};
            }),
            api::ERROR_INVALID_BINDINGS)
      << "按钮编号超出范围";
  EXPECT_EQ(nlohmann::json(*rig.registry->bindings()), nlohmann::json(good)) << "拒绝之后绑定保持原样";
  EXPECT_TRUE(rig.events->events.empty());
}

TEST(InputBindingRegistry, SavesTheBindingsAndReadsThemBack) {
  const ember::test::TempDir directory;
  const auto file = directory.path() / "config" / "input-bindings.json";
  {
    Rig rig(file);
    auto slots = *rig.registry->bindings();
    slots[1].gamepad = assignment("Demo Pad", 1);
    slots[1].keys = {{"start", "Numpad0"}};
    rig.registry->set(slots);
  }
  Rig again(file);
  const auto slots = *again.registry->bindings();
  ASSERT_TRUE(slots[1].gamepad.has_value());
  EXPECT_EQ(slots[1].gamepad->id, "Demo Pad");
  EXPECT_EQ(slots[1].gamepad->occurrence, 1);
  ASSERT_EQ(slots[1].gamepad->bindings.size(), 2U);
  EXPECT_EQ(slots[1].gamepad->bindings[1].direction, std::optional<std::string>("negative"));
  ASSERT_EQ(slots[1].keys.size(), 1U);
  EXPECT_EQ(slots[1].keys[0].code, "Numpad0");
  EXPECT_TRUE(again.errors->entries.empty());
}

TEST(InputBindingRegistry, AnUnusableFileFallsBackToTheDefaultsAndIsLogged) {
  const ember::test::TempDir directory;
  const auto file = directory.path() / "input-bindings.json";
  const auto write = [&](const std::string& text) {
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream << text;
  };
  const auto defaults = nlohmann::json(api::defaultPadSlots());
  for (const std::string& text : {std::string("垃圾"), std::string(R"({"version":2,"slots":[]})"), std::string(R"({"version":1,"slots":[{"keys":[]}]})"),
                                  std::string(R"({"version":1,"slots":[{"keys":[{"button":"a","code":"KeyX"}]},{"keys":[{"button":"a","code":"KeyX"}]}]})")}) {
    write(text);
    Rig rig(file);
    EXPECT_EQ(nlohmann::json(*rig.registry->bindings()), defaults) << text;
    ASSERT_EQ(rig.errors->entries.size(), 1U) << text;
    EXPECT_EQ(rig.errors->entries[0].code, "input.fileInvalid") << text;
  }
}

}  // namespace
