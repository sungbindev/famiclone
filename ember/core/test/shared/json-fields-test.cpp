/*
 * ------------------------------------------------------------------------
 * 名称：json-fields-test.cpp
 * 说明：JSON 字段读取与 Schema 生成代码的测试：类型、必填、长度、范围、未知字段与写出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "contracts/system/generated/system.hpp"
#include "shared/json/json-fields.hpp"

namespace {

namespace fields = ember::shared::json_fields;
namespace system_contract = ember::contracts::system;
using ember::shared::SchemaError;

TEST(JsonFields, ReadsStringWithLengthInCharacters) {
  EXPECT_EQ(fields::readString(nlohmann::json("模拟"), "p", 2, 2, {}), "模拟");
  EXPECT_THROW(fields::readString(nlohmann::json("如"), "p", 2, std::nullopt, {}), SchemaError);
  EXPECT_THROW(fields::readString(nlohmann::json(1), "p", std::nullopt, std::nullopt, {}), SchemaError);
  EXPECT_THROW(fields::readString(nlohmann::json("c"), "p", std::nullopt, std::nullopt, {"a", "b"}), SchemaError);
}

TEST(JsonFields, ReadsIntegerAndRejectsFractions) {
  EXPECT_EQ(fields::readInteger(nlohmann::json(3), "p", 0, 5), 3);
  EXPECT_EQ(fields::readInteger(nlohmann::json(3.0), "p", std::nullopt, std::nullopt), 3);
  EXPECT_THROW(fields::readInteger(nlohmann::json(3.5), "p", std::nullopt, std::nullopt), SchemaError);
  EXPECT_THROW(fields::readInteger(nlohmann::json(-1), "p", 0, std::nullopt), SchemaError);
}

TEST(JsonFields, ReadsArrayWithItemPaths) {
  const auto values = fields::readArray<bool>(nlohmann::json::array({true, false}), "p", 1, 3, [](const nlohmann::json& item, const std::string& path) {
    return fields::readBoolean(item, path);
  });
  EXPECT_EQ(values.size(), 2U);
  try {
    static_cast<void>(fields::readArray<bool>(nlohmann::json::array({true, 1}), "p", std::nullopt, std::nullopt, [](const nlohmann::json& item, const std::string& path) {
      return fields::readBoolean(item, path);
    }));
    FAIL() << "应抛出 SchemaError";
  } catch (const SchemaError& error) {
    EXPECT_NE(std::string(error.what()).find("p[1]"), std::string::npos);
  }
}

TEST(GeneratedSystem, ParsesAndWritesVersionResult) {
  const auto result = system_contract::parseSystemVersionResult(nlohmann::json{{"appVersion", "0.1.0"}, {"coreVersion", "1.0.0"}}, "result");
  EXPECT_EQ(result.appVersion, "0.1.0");
  EXPECT_EQ(result.coreVersion, "1.0.0");
  EXPECT_EQ(nlohmann::json(result), (nlohmann::json{{"appVersion", "0.1.0"}, {"coreVersion", "1.0.0"}}));
}

TEST(GeneratedSystem, RejectsMissingAndUnknownFields) {
  EXPECT_THROW(system_contract::parseSystemVersionResult(nlohmann::json{{"coreVersion", "1.0.0"}}, "result"), SchemaError);
  EXPECT_THROW(system_contract::parseSystemVersionResult(nlohmann::json{{"appVersion", ""}, {"coreVersion", "1.0.0"}}, "result"), SchemaError);
  EXPECT_THROW(system_contract::parseSystemVersionParams(nlohmann::json{{"extra", 1}}, "params"), SchemaError);
  EXPECT_THROW(system_contract::parseSystemVersionParams(nlohmann::json::array(), "params"), SchemaError);
  EXPECT_NO_THROW(system_contract::parseSystemVersionParams(nlohmann::json::object(), "params"));
}

}  // namespace
