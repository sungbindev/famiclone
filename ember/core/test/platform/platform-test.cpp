/*
 * ------------------------------------------------------------------------
 * 名称：platform-test.cpp
 * 说明：基础能力的测试：请求路由（含未登记请求被拒绝）、取消标记、任务池、时钟文本与错误日志文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "platform/logging/file-error-log.hpp"
#include "platform/requests/request-router.hpp"
#include "platform/threading/thread-pool.hpp"
#include "platform/time/system-clock.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using namespace ember;

/** 每个测试一个临时目录，结束时删除。 */
class TempDirectory {
 public:
  TempDirectory() : path_(std::filesystem::temp_directory_path() / ("ember-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
    std::filesystem::create_directories(path_);
  }
  ~TempDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }
  [[nodiscard]] const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
};

TEST(RequestRouter, DispatchesRegisteredRequests) {
  platform::RequestRouter router;
  router.add("demo", "demo.echo", [](const nlohmann::json& params, const platform::CancellationToken&) { return params; });
  EXPECT_TRUE(router.has("demo.echo"));
  EXPECT_EQ(router.dispatch("demo.echo", nlohmann::json{{"a", 1}}, platform::CancellationToken{}), (nlohmann::json{{"a", 1}}));
  EXPECT_EQ(router.names(), std::vector<std::string>{"demo.echo"});
}

TEST(RequestRouter, RejectsUnregisteredRequests) {
  platform::RequestRouter router;
  router.add("demo", "demo.one", [](const nlohmann::json&, const platform::CancellationToken&) { return nlohmann::json::object(); });
  EXPECT_FALSE(router.has("demo.missing"));
  try {
    static_cast<void>(router.dispatch("demo.missing", nlohmann::json::object(), platform::CancellationToken{}));
    FAIL() << "应抛出 CoreError";
  } catch (const shared::CoreError& error) {
    EXPECT_EQ(error.code(), shared::ERROR_UNKNOWN_REQUEST);
  }
}

TEST(RequestRouter, RejectsBadRegistrations) {
  platform::RequestRouter router;
  const auto handler = [](const nlohmann::json&, const platform::CancellationToken&) { return nlohmann::json::object(); };
  router.add("demo", "demo.one", handler);
  EXPECT_THROW(router.add("demo", "demo.one", handler), std::logic_error);
  EXPECT_THROW(router.add("demo", "other.one", handler), std::logic_error);
  EXPECT_THROW(router.add("demo", "demo.", handler), std::logic_error);
  EXPECT_THROW(router.add("demo", "demo.two", nullptr), std::logic_error);
}

TEST(CancellationToken, SharesStateBetweenCopies) {
  const platform::CancellationToken token;
  const auto copy = token;
  EXPECT_NO_THROW(copy.throwIfCancelled());
  token.cancel();
  EXPECT_TRUE(copy.isCancelled());
  EXPECT_THROW(copy.throwIfCancelled(), shared::CoreError);
}

TEST(ThreadPool, RunsQueuedTasksBeforeStopping) {
  std::atomic_int count = 0;
  platform::ThreadPool pool(2);
  for (int index = 0; index < 20; ++index) EXPECT_TRUE(pool.post([&count] { ++count; }));
  pool.stop();
  EXPECT_EQ(count.load(), 20);
  EXPECT_FALSE(pool.post([] {}));
}

TEST(FileErrorLog, AppendsJsonLinesUnderLocalDate) {
  const TempDirectory directory;
  auto clock = std::make_shared<platform::SystemClock>();
  const auto date = platform::formatLocalDate(clock->now());
  {
    platform::FileErrorLog log(directory.path(), clock);
    log.write({"demo", "core.internal", "出错了", {{"requestId", "r1"}}});
  }
  std::ifstream file(directory.path() / date / "errors.jsonl", std::ios::binary);
  std::string line;
  ASSERT_TRUE(std::getline(file, line));
  const auto entry = nlohmann::json::parse(line);
  EXPECT_EQ(entry["process"], "core");
  EXPECT_EQ(entry["module"], "demo");
  EXPECT_EQ(entry["message"], "出错了");
  EXPECT_EQ(entry["context"]["requestId"], "r1");
  EXPECT_TRUE(entry["stack"].is_null());
}

TEST(Clock, ParsesUtcIsoWrittenByFormatUtcIso) {
  const auto time = std::chrono::system_clock::from_time_t(1'700'000'000) + std::chrono::milliseconds(482);
  const auto parsed = platform::parseUtcIso(platform::formatUtcIso(time));
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(*parsed, time);
}

TEST(Clock, RejectsBadText) {
  EXPECT_FALSE(platform::parseUtcIso("不是时间").has_value());
  EXPECT_FALSE(platform::parseUtcIso("2026-13-01T00:00:00").has_value());
}

}  // namespace
