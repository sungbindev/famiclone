/*
 * ------------------------------------------------------------------------
 * 名称：platform-test.cpp
 * 说明：基础能力的测试：请求路由（含未登记请求被拒绝）、取消标记、任务池、时钟文本、错误日志文件与系统的文件变化监视。
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
#include <thread>

#include <gtest/gtest.h>

#include "platform/files/os-file-watch.hpp"
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

/** 轮询条件，最多等 5 秒。 */
template <typename Condition>
bool waitUntil(Condition&& condition) {
  for (int attempt = 0; attempt < 500; ++attempt) {
    if (condition()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return condition();
}

/** 以下用例需要系统的文件变化通知（Windows 与 macOS）。 */
#if defined(_WIN32) || defined(__APPLE__)
TEST(OsFileWatch, CallsBackWhenTheWatchedFileIsWrittenButNotForOtherFiles) {
  const TempDirectory directory;
  const auto watched = directory.path() / "watched.img";
  const auto other = directory.path() / "other.img";
  std::ofstream(watched, std::ios::binary) << "0123456789";
  std::ofstream(other, std::ios::binary) << "abc";
  std::atomic<int> calls{0};
  auto factory = platform::createOsFileWatchFactory();
  const auto text = watched.u8string();
  auto watcher = factory->watch(std::string(text.begin(), text.end()), [&calls] { ++calls; });
  ASSERT_NE(watcher, nullptr);
  {
    std::ofstream(other, std::ios::binary | std::ios::app) << "def";
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_EQ(calls.load(), 0) << "别的文件变了不回调";
  {
    std::fstream stream(watched, std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(3);
    stream.put('X');
  }
  EXPECT_TRUE(waitUntil([&] { return calls.load() > 0; })) << "被监视的文件改了就回调";
}

TEST(OsFileWatch, CallsBackWhenAnotherProgramReplacesTheFileByRenaming) {
  const TempDirectory directory;
  const auto watched = directory.path() / "watched.img";
  std::ofstream(watched, std::ios::binary) << "0123456789";
  std::atomic<int> calls{0};
  auto factory = platform::createOsFileWatchFactory();
  const auto text = watched.u8string();
  auto watcher = factory->watch(std::string(text.begin(), text.end()), [&calls] { ++calls; });
  ASSERT_NE(watcher, nullptr);
  const auto temporary = directory.path() / "watched.img.tmp";
  std::ofstream(temporary, std::ios::binary) << "ABCDEFGHIJ";
  std::filesystem::rename(temporary, watched);
  EXPECT_TRUE(waitUntil([&] { return calls.load() > 0; })) << "先写临时文件再改名覆盖的保存方式也能收到";
}

TEST(OsFileWatch, ADirectoryThatDoesNotExistCannotBeWatched) {
  auto factory = platform::createOsFileWatchFactory();
  EXPECT_EQ(factory->watch((std::filesystem::temp_directory_path() / "ember-no-such-directory" / "x.img").string(), [] {}), nullptr);
}
#endif

}  // namespace
