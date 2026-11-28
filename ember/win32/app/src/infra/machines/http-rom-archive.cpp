/*
 * ------------------------------------------------------------------------
 * 名称：http-rom-archive.cpp
 * 说明：ROM 包来源的网络实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-28
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "infra/machines/http-rom-archive.hpp"

#include <fstream>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "platform/network/https-client.hpp"
#include "platform/text/wide-text.hpp"

namespace ember::win32::infra {

namespace {

namespace fs = std::filesystem;

/** 下载总量的上限（字节）：ROM 包目前不到 8MB，留出裕量。 */
constexpr std::uint64_t kMaxPackBytes = 64ULL * 1024 * 1024;
/** 单个 ROM 条目的上限（字节）：最大的机型 ROM 是 2MB，留出裕量。 */
constexpr std::size_t kMaxEntryBytes = 4ULL * 1024 * 1024;

/** 条目名是不是包根下一个合格的 .rom 文件名：有扩展名，且不含路径分隔符、冒号与上级目录。 */
bool plainRomName(const std::string& name) {
  if (name.size() <= 4 || name.compare(name.size() - 4, 4, ".rom") != 0) return false;
  return name.find_first_of("/\\:") == std::string::npos && name.find("..") == std::string::npos;
}

}  // namespace

HttpRomArchive::HttpRomArchive(Options options, platform::UiThread& ui, contracts::ResourceArchiveOpener openArchive) : options_(std::move(options)), ui_(ui), openArchive_(std::move(openArchive)) {}

HttpRomArchive::~HttpRomArchive() {
  *alive_ = false;
  cancel_ = true;
  if (worker_.joinable()) worker_.join();
}

void HttpRomArchive::post(std::function<void()> task) {
  const std::weak_ptr<bool> weak = alive_;
  ui_.post([weak, task = std::move(task)] {
    const auto alive = weak.lock();
    if (alive && *alive) task();
  });
}

void HttpRomArchive::fetch(std::function<void(const contracts::RomArchiveResult&)> done) {
  if (busy_) {
    done({false, "已有一次下载在进行", {}});
    return;
  }
  busy_ = true;
  if (worker_.joinable()) worker_.join();
  cancel_ = false;
  worker_ = std::thread([this, done = std::move(done)]() mutable { run(std::move(done)); });
}

void HttpRomArchive::discard() {
  std::error_code error;
  fs::remove_all(options_.staging, error);
}

void HttpRomArchive::run(std::function<void(const contracts::RomArchiveResult&)> done) {
  // 结果回到界面线程，并解除“下载在进行”的标记
  const auto finish = [this, done](contracts::RomArchiveResult result) {
    post([this, done, result = std::move(result)] {
      busy_ = false;
      done(result);
    });
  };
  const auto fail = [&finish](const std::string& message) { finish({false, message, {}}); };

  std::error_code error;
  fs::create_directories(options_.staging, error);
  if (error) {
    fail("无法创建下载目录：" + error.message());
    return;
  }
  const fs::path pack = options_.staging / "roms.zip";
  const platform::HttpDownloadResult downloaded = platform::httpDownload(options_.url, pack, kMaxPackBytes, [this] { return cancel_.load(); });
  if (!downloaded.ok) {
    fs::remove(pack, error);
    fail("下载 ROM 包失败：" + downloaded.error);
    return;
  }

  // 读进内存后解包；包不是合格的 zip、或里面没有 ROM 时按失败处理
  std::ifstream stream(pack, std::ios::binary);
  const std::vector<unsigned char> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  stream.close();
  std::unique_ptr<contracts::ResourceArchive> archive;
  try {
    archive = openArchive_(data.data(), data.size());
  } catch (const std::runtime_error& problem) {
    fail(std::string("ROM 包的格式不对：") + problem.what());
    return;
  }
  std::vector<std::string> names;
  for (const std::string& name : archive->names()) {
    if (plainRomName(name)) names.push_back(name);
  }
  if (names.empty()) {
    fail("ROM 包里没有 ROM 文件");
    return;
  }

  const fs::path unpacked = options_.staging / "unpacked";
  fs::remove_all(unpacked, error);
  fs::create_directories(unpacked, error);
  if (error) {
    fail("无法创建解包目录：" + error.message());
    return;
  }
  for (const std::string& name : names) {
    const auto content = archive->read(name);
    if (!content) {
      fail("读不出 ROM 包里的文件：" + name);
      return;
    }
    if (content->size() > kMaxEntryBytes) {
      fail("ROM 包里的文件太大：" + name);
      return;
    }
    std::ofstream out(unpacked / platform::pathFromUtf8(name), std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(content->data()), static_cast<std::streamsize>(content->size()));
    out.close();
    if (!out) {
      fail("无法写出 ROM 文件：" + name);
      return;
    }
  }
  finish({true, {}, platform::pathToUtf8(unpacked)});
}

}  // namespace ember::win32::infra