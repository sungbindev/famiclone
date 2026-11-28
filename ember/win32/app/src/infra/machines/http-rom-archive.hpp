/*
 * ------------------------------------------------------------------------
 * 名称：http-rom-archive.hpp
 * 说明：ROM 包来源的网络实现：用 WinHTTP 下载 zip 到临时目录，解出里面的 .rom 文件，结果经界面线程给出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-28
 * 备注：包来自外部，是不可信输入：只认包根下的 .rom 条目，文件名里不能有路径分隔符、冒号与上级目录，条目大小与下载总量都有上限。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "contracts/machines/rom-archive-source.hpp"
#include "platform/threading/ui-thread.hpp"

namespace ember::win32::infra {

/** 从固定网址下载 ROM 包并解出目录。 */
class HttpRomArchive final : public contracts::RomArchiveSource {
 public:
  struct Options {
    /** ROM 包的网址（https，或指向 127.0.0.1 的 http）。 */
    std::string url;
    /** 下载与解包用的临时目录（用户数据目录下）；discard 删除它。 */
    std::filesystem::path staging;
  };

  HttpRomArchive(Options options, platform::UiThread& ui);
  ~HttpRomArchive() override;
  HttpRomArchive(const HttpRomArchive&) = delete;
  HttpRomArchive& operator=(const HttpRomArchive&) = delete;

  void fetch(std::function<void(const contracts::RomArchiveResult&)> done) override;
  void discard() override;

 private:
  void run(std::function<void(const contracts::RomArchiveResult&)> done);
  /** 把结果送回界面线程；对象已销毁时丢弃。 */
  void post(std::function<void()> task);

  Options options_;
  platform::UiThread& ui_;
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  std::atomic<bool> cancel_{false};
  std::thread worker_;
  /** 是否有下载在进行；只在界面线程读写。 */
  bool busy_{false};
};

}  // namespace ember::win32::infra