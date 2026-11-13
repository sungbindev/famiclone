/*
 * ------------------------------------------------------------------------
 * 名称：core-test.cpp
 * 说明：核心入口的测试：启动参数、system.version 请求、未登记请求被拒绝、错误码、未启动与停止后的请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "bootstrap/core.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::bootstrap::Core;

/** 一次请求的结果。 */
struct Reply {
  bool ok = false;
  nlohmann::json payload;
};

/** 同步等待请求结果。 */
Reply call(Core& core, const std::string& id, const std::string& name, const std::string& params) {
  std::promise<Reply> promise;
  auto future = promise.get_future();
  core.request(id, name, params, [&promise](bool ok, std::string payload) { promise.set_value({ok, nlohmann::json::parse(payload)}); });
  if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) throw std::runtime_error("请求超时");
  return future.get();
}

/** 临时用户数据目录。 */
class CoreTest : public testing::Test {
 protected:
  void SetUp() override {
    userData_ = std::filesystem::temp_directory_path() / ("ember-core-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(userData_, error);
  }
  [[nodiscard]] std::string options() const {
    const auto text = userData_.u8string();
    return nlohmann::json{{"userDataPath", std::string(text.begin(), text.end())}, {"appVersion", "9.8.7"}, {"workerCount", 2}}.dump();
  }

  std::filesystem::path userData_;
};

TEST_F(CoreTest, StartsAndAnswersSystemVersion) {
  Core core;
  const auto info = nlohmann::json::parse(core.start(options()));
  EXPECT_FALSE(info["coreVersion"].get<std::string>().empty());
  const auto& requests = info["requests"];
  EXPECT_EQ(requests.size(), 1U);
  EXPECT_NE(std::find(requests.begin(), requests.end(), nlohmann::json("system.version")), requests.end());

  const auto reply = call(core, "1", "system.version", "{}");
  ASSERT_TRUE(reply.ok);
  EXPECT_EQ(reply.payload["appVersion"], "9.8.7");
  EXPECT_EQ(reply.payload["coreVersion"], info["coreVersion"]);
  core.stop();
}

TEST_F(CoreTest, RejectsUnregisteredRequestsAndBadParams) {
  Core core;
  static_cast<void>(core.start(options()));
  const auto unknown = call(core, "1", "system.missing", "{}");
  EXPECT_FALSE(unknown.ok);
  EXPECT_EQ(unknown.payload["code"], "core.unknownRequest");
  const auto unknownModule = call(core, "2", "nothing.version", "{}");
  EXPECT_FALSE(unknownModule.ok);
  EXPECT_EQ(unknownModule.payload["code"], "core.unknownRequest");

  const auto invalid = call(core, "3", "system.version", R"({"extra":1})");
  EXPECT_FALSE(invalid.ok);
  EXPECT_EQ(invalid.payload["code"], "core.invalidParams");
  const auto notJson = call(core, "4", "system.version", "{");
  EXPECT_FALSE(notJson.ok);
  EXPECT_EQ(notJson.payload["code"], "core.invalidParams");
  core.stop();
}

TEST_F(CoreTest, RejectsBadStartOptions) {
  Core core;
  EXPECT_THROW(core.start("{"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"userDataPath":"x"})"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"appVersion":"1.0.0"})"), ember::shared::SchemaError);
  EXPECT_THROW(core.start(R"({"userDataPath":"x","appVersion":"1.0.0","other":1})"), ember::shared::SchemaError);
}

TEST_F(CoreTest, RefusesSecondStartAndRequestsAfterStop) {
  Core core;
  const auto before = call(core, "0", "system.version", "{}");
  EXPECT_FALSE(before.ok);
  EXPECT_EQ(before.payload["code"], "core.unavailable");

  static_cast<void>(core.start(options()));
  EXPECT_THROW(core.start(options()), ember::shared::CoreError);
  ASSERT_TRUE(call(core, "1", "system.version", "{}").ok);
  core.stop();
  const auto after = call(core, "2", "system.version", "{}");
  EXPECT_FALSE(after.ok);
  EXPECT_EQ(after.payload["code"], "core.unavailable");
}

}  // namespace
