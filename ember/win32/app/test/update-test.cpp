/*
 * ------------------------------------------------------------------------
 * 名称：update-test.cpp
 * 说明：应用升级的测试：清单规则（版本号比较、base64、清单格式与签名文字）、密码学（SHA-256、RSA 签名校验、公钥来自发布脚本的固定数据）、WinHTTP 下载（对本机服务取清单与下载文件、状态码、大小上限、取消、允许的网址）、
 *       升级引擎（发现新版本并下载、已是最新、签名不对、校验值不符、网络失败、已暂存的不重复下载、清理旧文件、不支持升级、没有暂存文件时不能重启）、升级服务（状态变化、超时、安装条件、订阅）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：引擎的网络传输用假的；WinHTTP 的测试用本机回环的 HTTP 服务；不访问外网，不启动新进程。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <map>
#include <thread>

#include "infra/updates/engine-app-updater.hpp"
#include "infra/updates/signed-exe-update-engine.hpp"
#include "platform/crypto/signature-verifier.hpp"
#include "platform/files/directory-access.hpp"
#include "platform/network/https-client.hpp"
#include "platform/network/local-http-server.hpp"
#include "platform/threading/ui-thread.hpp"
#include "shared/update/update-manifest.hpp"
#include "support/temp-directory.hpp"
#include "support/update-fixture.hpp"

namespace ember::win32 {
namespace {

namespace fixture = test::update_fixture;

/** 泵消息直到条件成立或超时（毫秒）。 */
template <typename Condition>
bool pumpUntil(Condition condition, int timeoutMilliseconds = 5000) {
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

void writeAll(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}

// ---- 清单规则 ----

TEST(UpdateManifest, ParsesVersionsAndComparesThem) {
  EXPECT_EQ(shared::parseVersion("1.2.3"), (std::array<std::uint32_t, 3>{1, 2, 3}));
  for (const char* bad : {"", "1", "1.2", "1.2.3.4", "01.2.3", "1.2.x", "a.b.c", "1..3", ".1.2", "1.2.", "-1.2.3", "1.2.3 "}) EXPECT_FALSE(shared::parseVersion(bad).has_value()) << bad;
  EXPECT_TRUE(shared::isNewerVersion("0.3.0", "0.2.9"));
  EXPECT_TRUE(shared::isNewerVersion("1.0.0", "0.99.99"));
  EXPECT_TRUE(shared::isNewerVersion("0.10.0", "0.9.0"));
  EXPECT_FALSE(shared::isNewerVersion("0.2.2", "0.2.2"));
  EXPECT_FALSE(shared::isNewerVersion("0.2.1", "0.2.2"));
  EXPECT_FALSE(shared::isNewerVersion("bad", "0.2.2"));
  EXPECT_FALSE(shared::isNewerVersion("0.3.0", "bad"));
}

TEST(UpdateManifest, DecodesBase64StrictlyAndEncodesHex) {
  EXPECT_EQ(shared::base64Decode("TWFu"), (std::vector<std::uint8_t>{'M', 'a', 'n'}));
  EXPECT_EQ(shared::base64Decode("TWE="), (std::vector<std::uint8_t>{'M', 'a'}));
  EXPECT_EQ(shared::base64Decode("TQ=="), (std::vector<std::uint8_t>{'M'}));
  for (const char* bad : {"", "TWF", "TW=u", "T===", "TWFu!", "=WFu", "TWE=TWFu", "TQ==TQ=="}) EXPECT_FALSE(shared::base64Decode(bad).has_value()) << bad;
  EXPECT_EQ(shared::toHex({0x00, 0xab, 0xff}), "00abff");
}

TEST(UpdateManifest, ParsesTheFixtureAndBuildsTheSignedMessage) {
  const auto manifest = shared::parseUpdateManifest(fixture::kManifestJson);
  ASSERT_TRUE(manifest.has_value());
  EXPECT_EQ(manifest->version, "9.9.9");
  EXPECT_EQ(manifest->file, "Ember-9.9.9.exe");
  EXPECT_EQ(manifest->size, 20U);
  EXPECT_EQ(manifest->sha256, fixture::kSha256);
  EXPECT_EQ(shared::updateSignedMessage(*manifest), fixture::kMessage);
  EXPECT_EQ(manifest->signature, *shared::base64Decode(fixture::kSignatureBase64));
}

TEST(UpdateManifest, RejectsMalformedManifests) {
  auto with = [](const std::string& key, const nlohmann::json& value) {
    auto json = nlohmann::json::parse(fixture::kManifestJson);
    json[key] = value;
    return json.dump();
  };
  EXPECT_FALSE(shared::parseUpdateManifest("").has_value());
  EXPECT_FALSE(shared::parseUpdateManifest("not json").has_value());
  EXPECT_FALSE(shared::parseUpdateManifest("[]").has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("version", "1.2")).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("file", "Ember-1.0.0.exe")).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("file", "../Ember-9.9.9.exe")).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("size", 0)).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("size", shared::kMaxUpdateFileBytes + 1)).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("size", "20")).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("sha256", "ABC")).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("sha256", std::string(64, 'G'))).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("sha256", std::string(64, 'A'))).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(with("signature", "!!!")).has_value());
  EXPECT_FALSE(shared::parseUpdateManifest(std::string(shared::kMaxManifestBytes + 1, ' ')).has_value());
  auto missing = nlohmann::json::parse(fixture::kManifestJson);
  missing.erase("signature");
  EXPECT_FALSE(shared::parseUpdateManifest(missing.dump()).has_value());
}

// ---- 密码学 ----

TEST(SignatureVerifier, Sha256MatchesKnownVectors) {
  const std::string abc = "abc";
  EXPECT_EQ(platform::sha256Hex(reinterpret_cast<const std::uint8_t*>(abc.data()), abc.size()), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  EXPECT_EQ(platform::sha256Hex(nullptr, 0), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SignatureVerifier, DigestOfAFileGivesSizeAndHashAndMissingFilesGiveNothing) {
  test::TempDirectory directory;
  writeAll(directory.path() / "a.bin", fixture::kContent);
  const auto digest = platform::digestFile(directory.path() / "a.bin");
  ASSERT_TRUE(digest.has_value());
  EXPECT_EQ(digest->sha256, fixture::kSha256);
  EXPECT_EQ(digest->size, fixture::kContent.size());
  EXPECT_FALSE(platform::digestFile(directory.path() / "missing.bin").has_value());
}

TEST(SignatureVerifier, VerifiesTheSignatureTheReleaseScriptMade) {
  const auto signature = *shared::base64Decode(fixture::kSignatureBase64);
  EXPECT_TRUE(platform::verifyRsaSha256(fixture::kModulus, fixture::kExponent, fixture::kMessage, signature));
}

TEST(SignatureVerifier, RejectsAlteredMessagesSignaturesAndKeys) {
  const auto signature = *shared::base64Decode(fixture::kSignatureBase64);
  EXPECT_FALSE(platform::verifyRsaSha256(fixture::kModulus, fixture::kExponent, fixture::kMessage + "x", signature));
  auto flipped = signature;
  flipped[10] ^= 0x01;
  EXPECT_FALSE(platform::verifyRsaSha256(fixture::kModulus, fixture::kExponent, fixture::kMessage, flipped));
  auto shortSignature = signature;
  shortSignature.pop_back();
  EXPECT_FALSE(platform::verifyRsaSha256(fixture::kModulus, fixture::kExponent, fixture::kMessage, shortSignature));
  auto otherModulus = fixture::kModulus;
  otherModulus.back() ^= 0x02;
  EXPECT_FALSE(platform::verifyRsaSha256(otherModulus, fixture::kExponent, fixture::kMessage, signature));
  EXPECT_FALSE(platform::verifyRsaSha256({}, fixture::kExponent, fixture::kMessage, signature));
  EXPECT_FALSE(platform::verifyRsaSha256(fixture::kModulus, {}, fixture::kMessage, signature));
}

// ---- WinHTTP ----

TEST(HttpClient, OnlyHttpsAndLoopbackHttpAreAllowed) {
  EXPECT_TRUE(platform::isAllowedHttpUrl("https://example.com/a"));
  EXPECT_TRUE(platform::isAllowedHttpUrl("http://127.0.0.1:8080/a"));
  EXPECT_TRUE(platform::isAllowedHttpUrl("http://127.0.0.1/a"));
  EXPECT_FALSE(platform::isAllowedHttpUrl("http://example.com/a"));
  EXPECT_FALSE(platform::isAllowedHttpUrl("http://127.0.0.1.evil.com/a"));
  EXPECT_FALSE(platform::isAllowedHttpUrl("ftp://127.0.0.1/a"));
  EXPECT_FALSE(platform::isAllowedHttpUrl(""));
  EXPECT_FALSE(platform::httpGet("http://example.com/", 100).ok);
}

TEST(HttpClient, GetsContentAndReportsStatusAndSizeLimit) {
  platform::LocalHttpServer server([](const std::string& target) {
    platform::HttpResponse response;
    if (target == "/ok") {
      response.contentType = "text/plain";
      response.body.assign({'h', 'e', 'l', 'l', 'o'});
    } else if (target == "/big") {
      response.body.assign(2000, 'x');
    } else {
      response.status = 404;
    }
    return response;
  });
  ASSERT_TRUE(server.start());
  const std::string base = "http://127.0.0.1:" + std::to_string(server.port());
  const auto ok = platform::httpGet(base + "/ok", 100);
  ASSERT_TRUE(ok.ok) << ok.error;
  EXPECT_EQ(std::string(ok.body.begin(), ok.body.end()), "hello");
  const auto missing = platform::httpGet(base + "/missing", 100);
  EXPECT_FALSE(missing.ok);
  EXPECT_EQ(missing.status, 404);
  EXPECT_NE(missing.error.find("404"), std::string::npos);
  const auto big = platform::httpGet(base + "/big", 100);
  EXPECT_FALSE(big.ok);
  EXPECT_TRUE(big.body.empty());
}

TEST(HttpClient, DownloadsToAFileAndHonoursLimitAndCancel) {
  platform::LocalHttpServer server([](const std::string& target) {
    platform::HttpResponse response;
    if (target == "/file") response.body.assign(300000, 'z');
    else response.status = 404;
    return response;
  });
  ASSERT_TRUE(server.start());
  const std::string base = "http://127.0.0.1:" + std::to_string(server.port());
  test::TempDirectory directory;
  const auto file = directory.path() / "file.bin";
  const auto ok = platform::httpDownload(base + "/file", file, 1000000, [] { return false; });
  ASSERT_TRUE(ok.ok) << ok.error;
  EXPECT_EQ(std::filesystem::file_size(file), 300000U);
  EXPECT_FALSE(platform::httpDownload(base + "/file", directory.path() / "small.bin", 1000, [] { return false; }).ok);
  EXPECT_FALSE(platform::httpDownload(base + "/file", directory.path() / "cancel.bin", 1000000, [] { return true; }).ok);
  EXPECT_FALSE(platform::httpDownload(base + "/missing", directory.path() / "missing.bin", 1000, [] { return false; }).ok);
}

TEST(HttpClient, UnreachableServerGivesAnError) {
  const auto result = platform::httpGet("http://127.0.0.1:1/x", 100);
  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.error.empty());
}

// ---- 升级引擎 ----

/** 假的网络传输：清单与文件内容可设，记下下载次数。 */
class FakeTransport final : public contracts::UpdateTransport {
 public:
  bool manifestOk{true};
  std::string manifestError{"无法连接服务器"};
  std::string manifest{fixture::kManifestJson};
  std::string content{fixture::kContent};
  std::string downloadError;
  int downloads{0};
  std::vector<std::string> urls;

  contracts::TransportGetResult get(const std::string& url, std::size_t) override {
    urls.push_back(url);
    contracts::TransportGetResult result;
    result.ok = manifestOk;
    result.error = manifestError;
    result.body.assign(manifest.begin(), manifest.end());
    return result;
  }
  std::string download(const std::string& url, const std::filesystem::path& file, std::uint64_t, const std::function<bool()>&) override {
    urls.push_back(url);
    ++downloads;
    if (!downloadError.empty()) return downloadError;
    writeAll(file, content);
    return {};
  }
};

/** 记下回调的结果。 */
struct Outcome {
  bool available{false};
  bool notAvailable{false};
  std::string downloaded;
  std::string failed;
  bool finished() const { return notAvailable || !downloaded.empty() || !failed.empty(); }

  contracts::UpdateEngineHandlers handlers() {
    contracts::UpdateEngineHandlers result;
    result.available = [this] { available = true; };
    result.notAvailable = [this] { notAvailable = true; };
    result.downloaded = [this](const std::string& version) { downloaded = version; };
    result.failed = [this](const std::string& message) { failed = message; };
    return result;
  }
};

struct EngineFixture {
  platform::UiThread ui;
  FakeTransport transport;
  test::TempDirectory directory;
  bool quit{false};

  [[nodiscard]] infra::SignedExeUpdateEngine::Options options(bool supported = true, const std::string& current = "0.2.2") {
    infra::SignedExeUpdateEngine::Options result;
    result.supported = supported;
    result.feedUrl = "https://example.com/updates";
    result.currentVersion = current;
    result.stagingDirectory = directory.path() / "updates";
    result.executable = directory.path() / "Ember.exe";
    result.publicModulus = fixture::kModulus;
    result.publicExponent = fixture::kExponent;
    result.quit = [this] { quit = true; };
    return result;
  }
};

TEST(SignedExeUpdateEngine, FindsANewerVersionDownloadsAndStagesIt) {
  EngineFixture f;
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_TRUE(outcome.available);
  EXPECT_EQ(outcome.downloaded, "9.9.9");
  EXPECT_EQ(readAll(f.directory.path() / "updates" / "Ember-9.9.9.exe"), fixture::kContent);
  EXPECT_FALSE(std::filesystem::exists(f.directory.path() / "updates" / "Ember-9.9.9.exe.part"));
  EXPECT_EQ(f.transport.urls, (std::vector<std::string>{"https://example.com/updates/latest.json", "https://example.com/updates/Ember-9.9.9.exe"}));
}

TEST(SignedExeUpdateEngine, SameOrOlderVersionIsNotAvailable) {
  EngineFixture f;
  infra::SignedExeUpdateEngine engine(f.options(true, "9.9.9"), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_TRUE(outcome.notAvailable);
  EXPECT_FALSE(outcome.available);
  EXPECT_EQ(f.transport.downloads, 0);
}

TEST(SignedExeUpdateEngine, UpToDateRemovesTheStagedFilesOfEarlierChecks) {
  EngineFixture f;
  const auto staging = f.directory.path() / "updates";
  writeAll(staging / "Ember-9.9.9.exe", fixture::kContent);
  writeAll(staging / "notes.txt", "keep");
  infra::SignedExeUpdateEngine engine(f.options(true, "9.9.9"), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_TRUE(outcome.notAvailable);
  EXPECT_FALSE(std::filesystem::exists(staging / "Ember-9.9.9.exe"));
  EXPECT_TRUE(std::filesystem::exists(staging / "notes.txt"));
}

TEST(SignedExeUpdateEngine, RefusesAManifestWhoseSignatureDoesNotMatch) {
  EngineFixture f;
  // 把清单里的版本改了：格式合格，但签名对不上
  auto json = nlohmann::json::parse(fixture::kManifestJson);
  json["version"] = "9.9.8";
  json["file"] = "Ember-9.9.8.exe";
  f.transport.manifest = json.dump();
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_NE(outcome.failed.find("签名"), std::string::npos);
  EXPECT_EQ(f.transport.downloads, 0);
}

TEST(SignedExeUpdateEngine, RefusesAMalformedManifest) {
  EngineFixture f;
  f.transport.manifest = "{\"version\": 1}";
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_NE(outcome.failed.find("格式"), std::string::npos);
}

TEST(SignedExeUpdateEngine, ReportsNetworkFailures) {
  EngineFixture f;
  f.transport.manifestOk = false;
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_NE(outcome.failed.find("无法连接服务器"), std::string::npos);
}

TEST(SignedExeUpdateEngine, DiscardsADownloadWhoseHashDoesNotMatch) {
  EngineFixture f;
  f.transport.content = "TAMPERED-CONTENT-9.9!";
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_NE(outcome.failed.find("对不上"), std::string::npos);
  EXPECT_FALSE(std::filesystem::exists(f.directory.path() / "updates" / "Ember-9.9.9.exe"));
  EXPECT_FALSE(std::filesystem::exists(f.directory.path() / "updates" / "Ember-9.9.9.exe.part"));
}

TEST(SignedExeUpdateEngine, ReportsDownloadFailures) {
  EngineFixture f;
  f.transport.downloadError = "连接中断";
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_NE(outcome.failed.find("连接中断"), std::string::npos);
  EXPECT_TRUE(outcome.available);
}

TEST(SignedExeUpdateEngine, AlreadyStagedFileIsNotDownloadedAgainAndOldFilesAreCleaned) {
  EngineFixture f;
  const auto staging = f.directory.path() / "updates";
  writeAll(staging / "Ember-9.9.9.exe", fixture::kContent);
  writeAll(staging / "Ember-9.9.8.exe", "old");
  writeAll(staging / "Ember-9.9.7.exe.part", "partial");
  writeAll(staging / "notes.txt", "keep");
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_EQ(outcome.downloaded, "9.9.9");
  EXPECT_EQ(f.transport.downloads, 0);
  EXPECT_FALSE(std::filesystem::exists(staging / "Ember-9.9.8.exe"));
  EXPECT_FALSE(std::filesystem::exists(staging / "Ember-9.9.7.exe.part"));
  EXPECT_TRUE(std::filesystem::exists(staging / "notes.txt"));
}

TEST(SignedExeUpdateEngine, StagedFileWithTheWrongContentIsReplaced) {
  EngineFixture f;
  const auto staging = f.directory.path() / "updates";
  writeAll(staging / "Ember-9.9.9.exe", "WRONG");
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  Outcome outcome;
  engine.check(outcome.handlers());
  ASSERT_TRUE(pumpUntil([&] { return outcome.finished(); }));
  EXPECT_EQ(f.transport.downloads, 1);
  EXPECT_EQ(readAll(staging / "Ember-9.9.9.exe"), fixture::kContent);
}

TEST(SignedExeUpdateEngine, UnsupportedEngineDoesNothingAndCannotRestart) {
  EngineFixture f;
  infra::SignedExeUpdateEngine engine(f.options(false), f.ui, f.transport);
  EXPECT_FALSE(engine.supported());
  Outcome outcome;
  engine.check(outcome.handlers());
  pumpUntil([&] { return outcome.finished(); }, 100);
  EXPECT_FALSE(outcome.finished());
  EXPECT_TRUE(f.transport.urls.empty());
  EXPECT_FALSE(engine.restart());
  EXPECT_FALSE(f.quit);
}

TEST(SignedExeUpdateEngine, CannotRestartBeforeAnythingIsStaged) {
  EngineFixture f;
  infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
  EXPECT_FALSE(engine.restart());
  EXPECT_FALSE(f.quit);
}

TEST(SignedExeUpdateEngine, DestroyingTheEngineBeforeTheResultIsDeliveredIsSafe) {
  EngineFixture f;
  Outcome outcome;
  {
    infra::SignedExeUpdateEngine engine(f.options(), f.ui, f.transport);
    engine.check(outcome.handlers());
  }
  pumpUntil([&] { return outcome.finished(); }, 200);
  EXPECT_FALSE(outcome.finished());
}

// ---- 目录写入检查 ----

TEST(DirectoryAccess, CanCreateFilesOnlyInExistingWritableDirectoriesAndLeavesNothingBehind) {
  test::TempDirectory directory;
  EXPECT_TRUE(platform::canCreateFilesIn(directory.path()));
  EXPECT_TRUE(std::filesystem::is_empty(directory.path()));
  EXPECT_FALSE(platform::canCreateFilesIn(directory.path() / "missing"));
  EXPECT_FALSE(platform::canCreateFilesIn(std::filesystem::path()));
  writeAll(directory.path() / "file.txt", "x");
  EXPECT_FALSE(platform::canCreateFilesIn(directory.path() / "file.txt"));
}

// ---- 升级服务 ----

/** 假的升级引擎：记下回调，由测试触发。 */
class FakeEngine final : public contracts::UpdateEngine {
 public:
  bool isSupported{true};
  bool restartResult{true};
  int restarts{0};
  int checks{0};
  contracts::UpdateEngineHandlers handlers;

  [[nodiscard]] bool supported() const override { return isSupported; }
  void check(contracts::UpdateEngineHandlers next) override {
    ++checks;
    handlers = std::move(next);
  }
  bool restart() override {
    ++restarts;
    return restartResult;
  }
};

TEST(EngineAppUpdater, StartsUnsupportedOrIdleAndRefusesToCheckWhenUnsupported) {
  platform::UiThread ui;
  FakeEngine unsupported;
  unsupported.isSupported = false;
  infra::EngineAppUpdater a(unsupported, ui);
  EXPECT_EQ(a.status().state, "unsupported");
  EXPECT_EQ(a.check().state, "unsupported");
  EXPECT_EQ(unsupported.checks, 0);
  FakeEngine engine;
  infra::EngineAppUpdater b(engine, ui);
  EXPECT_EQ(b.status().state, "idle");
}

TEST(EngineAppUpdater, WalksThroughCheckingDownloadingAndReady) {
  platform::UiThread ui;
  FakeEngine engine;
  infra::EngineAppUpdater updater(engine, ui);
  std::vector<std::string> states;
  const auto stop = updater.subscribe([&](const contracts::UpdateStatus& status) { states.push_back(status.state + (status.version ? ":" + *status.version : "")); });
  EXPECT_EQ(updater.check().state, "checking");
  // 检查中再检查不会重复
  EXPECT_EQ(updater.check().state, "checking");
  EXPECT_EQ(engine.checks, 1);
  engine.handlers.available();
  engine.handlers.downloaded("1.2.3");
  EXPECT_EQ(updater.status().state, "ready");
  EXPECT_EQ(updater.status().version, "1.2.3");
  EXPECT_EQ(states, (std::vector<std::string>{"checking", "downloading", "ready:1.2.3"}));
  // 已经下载好之后不再检查，也不再接受引擎的事件
  EXPECT_EQ(updater.check().state, "ready");
  engine.handlers.failed("迟到的失败");
  EXPECT_EQ(updater.status().state, "ready");
  stop();
}

TEST(EngineAppUpdater, NotAvailableAndFailureCanBeCheckedAgain) {
  platform::UiThread ui;
  FakeEngine engine;
  infra::EngineAppUpdater updater(engine, ui);
  updater.check();
  engine.handlers.notAvailable();
  EXPECT_EQ(updater.status().state, "upToDate");
  updater.check();
  engine.handlers.failed("网络不通");
  EXPECT_EQ(updater.status().state, "error");
  EXPECT_EQ(updater.status().message, "网络不通");
  EXPECT_EQ(updater.check().state, "checking");
  EXPECT_EQ(engine.checks, 3);
}

TEST(EngineAppUpdater, CheckingTimesOutButDownloadingDoesNot) {
  platform::UiThread ui;
  FakeEngine engine;
  infra::EngineAppUpdater updater(engine, ui, 30);
  updater.check();
  ASSERT_TRUE(pumpUntil([&] { return updater.status().state == "error"; }));
  EXPECT_NE(updater.status().message->find("超时"), std::string::npos);
  updater.check();
  engine.handlers.available();
  pumpUntil([] { return false; }, 120);
  EXPECT_EQ(updater.status().state, "downloading");
}

TEST(EngineAppUpdater, InstallOnlyWhenReadyAndPassesTheEnginesResult) {
  platform::UiThread ui;
  FakeEngine engine;
  infra::EngineAppUpdater updater(engine, ui);
  EXPECT_FALSE(updater.install());
  EXPECT_EQ(engine.restarts, 0);
  updater.check();
  engine.handlers.downloaded("2.0.0");
  engine.restartResult = false;
  EXPECT_FALSE(updater.install());
  engine.restartResult = true;
  EXPECT_TRUE(updater.install());
  EXPECT_EQ(engine.restarts, 2);
}

TEST(EngineAppUpdater, DestroyedUpdaterIgnoresLateEngineCallbacks) {
  platform::UiThread ui;
  FakeEngine engine;
  {
    infra::EngineAppUpdater updater(engine, ui);
    updater.check();
  }
  EXPECT_NO_THROW(engine.handlers.downloaded("1.0.0"));
  EXPECT_NO_THROW(engine.handlers.failed("x"));
}

}  // namespace
}  // namespace ember::win32
