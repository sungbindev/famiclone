/*
 * ------------------------------------------------------------------------
 * 名称：platform-test.cpp
 * 说明：平台层的测试：文字转换、界面线程的任务投递、JSON 文件存储、错误日志、单实例。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不弹出任何窗口；界面线程的测试在当前线程里泵消息，单实例的测试在同一进程里创建两个对象模拟先后启动的两个进程。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <thread>

#include "platform/files/json-file-reader.hpp"
#include "platform/files/json-file-store.hpp"
#include "platform/files/known-folders.hpp"
#include "platform/logging/file-error-logger.hpp"
#include "platform/process/single-instance.hpp"
#include "platform/text/wide-text.hpp"
#include "platform/theme/system-theme.hpp"
#include "platform/threading/ui-thread.hpp"
#include "bootstrap/resource-locations.hpp"
#include "support/temp-directory.hpp"

namespace {

using namespace ember::win32;
using nlohmann::json;

/** 泵消息直到条件成立或超时（毫秒）。 */
template <typename Condition>
bool pumpUntil(Condition condition, int timeoutMilliseconds = 3000) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMilliseconds);
  while (!condition()) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    platform::UiThread::pumpMessages();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

std::string readAll(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

TEST(WideText, RoundTripsChineseAndEmojiAndPaths) {
  const std::string text = "中文路径\\卡带 🎮.nes";
  EXPECT_EQ(platform::narrow(platform::widen(text)), text);
  EXPECT_EQ(platform::pathToUtf8(platform::pathFromUtf8(text)), text);
  EXPECT_TRUE(platform::widen("").empty());
}

TEST(WideText, InvalidUtf8GivesEmpty) { EXPECT_TRUE(platform::widen("\xFF\xFE").empty()); }

TEST(KnownFolders, DefaultUserDataDirectoryIsEmberUnderAppData) {
  const auto directory = platform::defaultUserDataDirectory();
  ASSERT_FALSE(directory.empty());
  EXPECT_EQ(directory.filename().wstring(), L"Ember");
  EXPECT_EQ(directory.parent_path(), platform::roamingAppDataDirectory());
  EXPECT_FALSE(platform::executableDirectory().empty());
}

TEST(SystemTheme, ReturnsAValueWithoutThrowing) { static_cast<void>(platform::systemUsesLightTheme()); }

TEST(UiThread, PostFromAnotherThreadRunsOnTheUiThread) {
  platform::UiThread ui;
  std::thread::id ranOn;
  bool done = false;
  std::thread worker([&] { ui.post([&] { ranOn = std::this_thread::get_id(); done = true; }); });
  ASSERT_TRUE(pumpUntil([&] { return done; }));
  worker.join();
  EXPECT_EQ(ranOn, std::this_thread::get_id());
  EXPECT_TRUE(ui.isCurrent());
}

TEST(UiThread, PostedTasksRunInOrder) {
  platform::UiThread ui;
  std::string order;
  for (char c : std::string("abc")) ui.post([&order, c] { order += c; });
  ASSERT_TRUE(pumpUntil([&] { return order.size() == 3; }));
  EXPECT_EQ(order, "abc");
}

TEST(UiThread, DelayedTaskRunsOnceAndCanBeCancelled) {
  platform::UiThread ui;
  int ran = 0;
  int cancelled = 0;
  ui.postDelayed(20, [&] { ++ran; });
  const unsigned id = ui.postDelayed(20, [&] { ++cancelled; });
  ui.cancelDelayed(id);
  ASSERT_TRUE(pumpUntil([&] { return ran == 1; }));
  pumpUntil([] { return false; }, 100);
  EXPECT_EQ(ran, 1);
  EXPECT_EQ(cancelled, 0);
}

TEST(JsonFileStore, MissingFileReadsAsEmptyWithoutReportingAnError) {
  test::TempDirectory directory;
  platform::UiThread ui;
  std::vector<std::string> errors;
  platform::JsonFileStore store(directory.path() / "a.json", ui, [&](const std::string& message) { errors.push_back(message); });
  EXPECT_FALSE(store.read());
  EXPECT_TRUE(errors.empty());
}

TEST(JsonFileStore, BrokenJsonReadsAsEmptyAndIsReported) {
  test::TempDirectory directory;
  platform::UiThread ui;
  { std::ofstream(directory.path() / "a.json") << "{ not json"; }
  std::vector<std::string> errors;
  platform::JsonFileStore store(directory.path() / "a.json", ui, [&](const std::string& message) { errors.push_back(message); });
  EXPECT_FALSE(store.read());
  EXPECT_EQ(errors.size(), 1u);
}

TEST(JsonFileStore, FlushWritesImmediatelyAndRoundTripsChinese) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "config" / "a.json";
  platform::JsonFileStore store(file, ui);
  store.write({{"name", "卡带"}});
  EXPECT_FALSE(std::filesystem::exists(file));
  store.flush();
  ASSERT_TRUE(std::filesystem::exists(file));
  EXPECT_FALSE(std::filesystem::exists(file.string() + ".tmp"));
  platform::JsonFileStore reader(file, ui);
  const auto value = reader.read();
  ASSERT_TRUE(value);
  EXPECT_EQ((*value)["name"], "卡带");
}

TEST(JsonFileStore, DebouncedWritesCollapseIntoTheLastValue) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "a.json";
  platform::JsonFileStore store(file, ui, {}, 30);
  for (int i = 0; i < 5; ++i) store.write({{"n", i}});
  EXPECT_FALSE(std::filesystem::exists(file));
  ASSERT_TRUE(pumpUntil([&] { return std::filesystem::exists(file); }));
  EXPECT_EQ(json::parse(readAll(file))["n"], 4);
}

TEST(JsonFileStore, DestructorWritesPendingContent) {
  test::TempDirectory directory;
  platform::UiThread ui;
  const auto file = directory.path() / "a.json";
  { platform::JsonFileStore store(file, ui, {}, 10000); store.write({{"n", 1}}); }
  EXPECT_EQ(json::parse(readAll(file))["n"], 1);
}

TEST(FileErrorLogger, AppendsOneJsonLinePerErrorUnderTheLocalDate) {
  test::TempDirectory directory;
  platform::FileErrorLogger logger(directory.path() / "logs", platform::FileErrorLogger::Clock{[] { return std::string("2026-11-19"); }, [] { return std::string("2026-11-19T01:02:03.004Z"); }});
  logger.write({std::string("system"), "core.internal", "出错了", {{"name", "x"}}});
  logger.write({std::nullopt, "core.unavailable", "第二条", json::object()});
  const std::string text = readAll(directory.path() / "logs" / "2026-11-19" / "errors.jsonl");
  std::vector<json> lines;
  std::size_t start = 0;
  while (start < text.size()) {
    const std::size_t end = text.find('\n', start);
    lines.push_back(json::parse(text.substr(start, end - start)));
    start = end + 1;
  }
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0]["process"], "main");
  EXPECT_EQ(lines[0]["module"], "system");
  EXPECT_EQ(lines[0]["message"], "出错了");
  EXPECT_EQ(lines[0]["time"], "2026-11-19T01:02:03.004Z");
  EXPECT_EQ(lines[0]["context"]["name"], "x");
  EXPECT_TRUE(lines[1]["module"].is_null());
  EXPECT_TRUE(lines[1]["stack"].is_null());
}

TEST(FileErrorLogger, NeverThrowsEvenWhenTheDirectoryCannotBeCreated) {
  platform::FileErrorLogger logger("Z:\\no\\such\\drive\\logs");
  EXPECT_NO_THROW(logger.write({std::nullopt, "core.internal", "x", json::object()}));
}

TEST(SingleInstance, SecondInstanceIsNotifiedAndTheFirstKeepsRunning) {
  test::TempDirectory directory;
  int activated = 0;
  platform::SingleInstance first(directory.path(), [&] { ++activated; });
  ASSERT_EQ(first.acquire(), platform::SingleInstanceResult::Acquired);
  EXPECT_TRUE(std::filesystem::exists(directory.path() / "lockfile"));

  bool secondActivated = false;
  platform::SingleInstance second(directory.path(), [&] { secondActivated = true; });
  EXPECT_EQ(second.acquire(), platform::SingleInstanceResult::NotifiedExisting);
  EXPECT_EQ(activated, 1);
  EXPECT_FALSE(secondActivated);
}

TEST(SingleInstance, LockIsReleasedWhenTheFirstInstanceEnds) {
  test::TempDirectory directory;
  {
    platform::SingleInstance first(directory.path(), [] {});
    ASSERT_EQ(first.acquire(), platform::SingleInstanceResult::Acquired);
  }
  EXPECT_FALSE(std::filesystem::exists(directory.path() / "lockfile"));
  platform::SingleInstance next(directory.path(), [] {});
  EXPECT_EQ(next.acquire(), platform::SingleInstanceResult::Acquired);
}

TEST(SingleInstance, DifferentUserDataDirectoriesDoNotInterfere) {
  test::TempDirectory a;
  test::TempDirectory b;
  platform::SingleInstance first(a.path(), [] {});
  platform::SingleInstance other(b.path(), [] {});
  EXPECT_EQ(first.acquire(), platform::SingleInstanceResult::Acquired);
  EXPECT_EQ(other.acquire(), platform::SingleInstanceResult::Acquired);
}

TEST(JsonFileReader, ReadsValidFilesAndGivesEmptyForMissingOrBrokenOnes) {
  test::TempDirectory directory;
  EXPECT_FALSE(platform::readJsonFile(directory.path() / "missing.json"));
  { std::ofstream(directory.path() / "bad.json") << "{ nope"; }
  EXPECT_FALSE(platform::readJsonFile(directory.path() / "bad.json"));
  { std::ofstream(directory.path() / "good.json") << R"({"name":"卡带","n":[1,2]})"; }
  const auto value = platform::readJsonFile(directory.path() / "good.json");
  ASSERT_TRUE(value);
  EXPECT_EQ((*value)["name"], "卡带");
  EXPECT_EQ((*value)["n"].size(), 2u);
}

TEST(ResourceLocations, PackagedLayoutUsesTheResourcesDirectoryNextToTheExecutable) {
  test::TempDirectory directory;
  const auto exe = directory.path() / "app";
  std::filesystem::create_directories(exe / "resources" / "plugins");
  std::filesystem::create_directories(exe / "resources" / "bios");
  std::filesystem::create_directories(exe / "resources" / "win32-x64");
  { std::ofstream(exe / "resources" / "win32-x64" / "ffmpeg.exe") << "x"; }
  const auto found = bootstrap::locateResources(exe);
  ASSERT_TRUE(found.plugins && found.bios && found.ffmpeg);
  EXPECT_EQ(platform::pathFromUtf8(*found.plugins), (exe / "resources" / "plugins").lexically_normal());
  EXPECT_EQ(platform::pathFromUtf8(*found.ffmpeg).filename().wstring(), L"ffmpeg.exe");
}

TEST(ResourceLocations, DevelopmentLayoutFindsBuildAndRepositoryDirectories) {
  test::TempDirectory directory;
  // 仓库根目录\build\ember\win32\app\Debug 是可执行文件所在的目录
  const auto exe = directory.path() / "build" / "ember" / "win32" / "app" / "Debug";
  std::filesystem::create_directories(exe);
  std::filesystem::create_directories(directory.path() / "build" / "ember" / "plugins");
  std::filesystem::create_directories(directory.path() / "ember" / "bios");
  std::filesystem::create_directories(directory.path() / "ember" / "core" / "third_party" / "ffmpeg" / "bin" / "win32-x64");
  { std::ofstream(directory.path() / "ember" / "core" / "third_party" / "ffmpeg" / "bin" / "win32-x64" / "ffmpeg.exe") << "x"; }
  const auto found = bootstrap::locateResources(exe);
  ASSERT_TRUE(found.plugins && found.bios && found.ffmpeg);
  EXPECT_EQ(platform::pathFromUtf8(*found.plugins), (directory.path() / "build" / "ember" / "plugins").lexically_normal());
  EXPECT_EQ(platform::pathFromUtf8(*found.bios), (directory.path() / "ember" / "bios").lexically_normal());
}

TEST(ResourceLocations, MissingResourcesAreEmpty) {
  test::TempDirectory directory;
  const auto found = bootstrap::locateResources(directory.path() / "a" / "b" / "c" / "d" / "e");
  EXPECT_FALSE(found.plugins);
  EXPECT_FALSE(found.bios);
  EXPECT_FALSE(found.ffmpeg);
}

}  // namespace
