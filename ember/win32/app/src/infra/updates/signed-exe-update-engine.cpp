/*
 * ------------------------------------------------------------------------
 * 名称：signed-exe-update-engine.cpp
 * 说明：签名程序文件的升级引擎的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/updates/signed-exe-update-engine.hpp"

#include <system_error>

#include "platform/crypto/signature-verifier.hpp"
#include "platform/process/process-control.hpp"
#include "platform/text/wide-text.hpp"
#include "shared/update/update-manifest.hpp"

namespace ember::win32::infra {

namespace {

namespace fs = std::filesystem;

constexpr const char* kManifestName = "/latest.json";
constexpr const char* kPartSuffix = ".part";

/** 暂存目录里的文件名是不是 Ember-<版本号>.exe 或它的 .part；是时取出版本号。 */
std::string stagedVersion(std::string name) {
  if (name.size() > 5 && name.compare(name.size() - 5, 5, kPartSuffix) == 0) name.resize(name.size() - 5);
  if (name.rfind("Ember-", 0) != 0 || name.size() <= 10 || name.compare(name.size() - 4, 4, ".exe") != 0) return {};
  return name.substr(6, name.size() - 10);
}

}  // namespace

SignedExeUpdateEngine::SignedExeUpdateEngine(Options options, platform::UiThread& ui, contracts::UpdateTransport& transport) : options_(std::move(options)), ui_(ui), transport_(transport) {}

SignedExeUpdateEngine::~SignedExeUpdateEngine() {
  *alive_ = false;
  cancel_ = true;
  if (worker_.joinable()) worker_.join();
}

void SignedExeUpdateEngine::post(std::function<void()> task) {
  const std::weak_ptr<bool> weak = alive_;
  ui_.post([weak, task = std::move(task)] {
    const auto alive = weak.lock();
    if (alive && *alive) task();
  });
}

void SignedExeUpdateEngine::check(contracts::UpdateEngineHandlers handlers) {
  if (!options_.supported) return;
  if (worker_.joinable()) worker_.join();
  cancel_ = false;
  worker_ = std::thread([this, handlers = std::move(handlers)]() mutable { run(std::move(handlers)); });
}

void SignedExeUpdateEngine::cleanStaging(const std::string& keepFile) const {
  std::error_code error;
  if (!fs::is_directory(options_.stagingDirectory, error)) return;
  for (const auto& entry : fs::directory_iterator(options_.stagingDirectory, error)) {
    const std::string name = platform::pathToUtf8(entry.path().filename());
    if (stagedVersion(name).empty() || name == keepFile) continue;
    fs::remove(entry.path(), error);
  }
}

void SignedExeUpdateEngine::run(contracts::UpdateEngineHandlers handlers) {
  const auto fail = [this, &handlers](const std::string& message) {
    post([failed = handlers.failed, message] {
      if (failed) failed(message);
    });
  };

  // 清单
  const contracts::TransportGetResult fetched = transport_.get(options_.feedUrl + kManifestName, shared::kMaxManifestBytes);
  if (!fetched.ok) {
    fail("无法获取升级信息：" + fetched.error);
    return;
  }
  const auto manifest = shared::parseUpdateManifest(std::string_view(reinterpret_cast<const char*>(fetched.body.data()), fetched.body.size()));
  if (!manifest) {
    fail("升级信息的格式不对，已忽略");
    return;
  }
  if (!platform::verifyRsaSha256(options_.publicModulus, options_.publicExponent, shared::updateSignedMessage(*manifest), manifest->signature)) {
    fail("升级信息的签名不正确，已忽略");
    return;
  }
  if (!shared::isNewerVersion(manifest->version, options_.currentVersion)) {
    // 已经是最新（含刚升级完）：以前暂存的新程序不再有用，清掉
    cleanStaging({});
    post([notAvailable = handlers.notAvailable] {
      if (notAvailable) notAvailable();
    });
    return;
  }
  post([available = handlers.available] {
    if (available) available();
  });

  // 新程序：已经暂存且校验通过的不重复下载
  std::error_code error;
  fs::create_directories(options_.stagingDirectory, error);
  cleanStaging(manifest->file);
  const fs::path staged = options_.stagingDirectory / platform::pathFromUtf8(manifest->file);
  const auto matches = [&manifest](const fs::path& file) {
    const auto digest = platform::digestFile(file);
    return digest && digest->size == manifest->size && digest->sha256 == manifest->sha256;
  };
  if (!matches(staged)) {
    const fs::path part = fs::path(staged.wstring() + L".part");
    const std::string downloadError = transport_.download(options_.feedUrl + "/" + manifest->file, part, manifest->size, [this] { return cancel_.load(); });
    if (!downloadError.empty()) {
      fs::remove(part, error);
      fail("下载新版本失败：" + downloadError);
      return;
    }
    if (!matches(part)) {
      fs::remove(part, error);
      fail("下载的新版本与升级信息对不上（大小或校验值不同），已丢弃");
      return;
    }
    fs::remove(staged, error);
    fs::rename(part, staged, error);
    if (error) {
      fs::remove(part, error);
      fail("无法保存下载的新版本：" + error.message());
      return;
    }
  }
  post([this, staged, version = manifest->version, downloaded = handlers.downloaded] {
    ready_ = staged;
    if (downloaded) downloaded(version);
  });
}

bool SignedExeUpdateEngine::restart() {
  if (ready_.empty() || options_.executable.empty()) return false;
  const std::vector<std::string> arguments{"--apply-update=" + platform::pathToUtf8(options_.executable), "--wait-pid=" + std::to_string(platform::currentProcessId())};
  if (!platform::startProcess(ready_, arguments)) return false;
  if (options_.quit) options_.quit();
  return true;
}

}  // namespace ember::win32::infra
