/*
 * ------------------------------------------------------------------------
 * 名称：signed-exe-update-engine.hpp
 * 说明：升级引擎：从升级地址取清单（latest.json），用写在程序里的 RSA 公钥校验清单的签名，发现比当前新的版本就下载新的程序文件，核对大小与 SHA-256 后放进暂存目录；重启时启动暂存的新程序，由它接替当前程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：检查与下载在一个后台线程里做，结果经界面线程回调；只接受比当前版本新的版本（不会降级）；签名不对、清单格式不对、文件大小或 SHA-256 对不上的一律丢弃并报告失败；
 *       暂存目录里只留这一次要用的新程序，其余旧版本与没下完的文件在每次检查时清理；规则见 docs/01-软件架构设计.md 第 11 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "contracts/system/update-engine.hpp"
#include "contracts/system/update-transport.hpp"
#include "platform/threading/ui-thread.hpp"

namespace ember::win32::infra {

/** 签名程序文件的升级引擎。 */
class SignedExeUpdateEngine final : public contracts::UpdateEngine {
 public:
  /** 引擎的设置。 */
  struct Options {
    /** 当前运行方式是否支持升级。 */
    bool supported{false};
    /** 升级地址：下面放 latest.json 与 Ember-<版本号>.exe，不带结尾的 /。 */
    std::string feedUrl;
    /** 当前程序的版本号。 */
    std::string currentVersion;
    /** 暂存新程序的目录。 */
    std::filesystem::path stagingDirectory;
    /** 当前程序的完整路径（新程序要覆盖它）。 */
    std::filesystem::path executable;
    /** 签名公钥（大端字节）。 */
    std::vector<std::uint8_t> publicModulus;
    std::vector<std::uint8_t> publicExponent;
    /** 启动新程序成功后调用，让当前程序退出。 */
    std::function<void()> quit;
  };

  SignedExeUpdateEngine(Options options, platform::UiThread& ui, contracts::UpdateTransport& transport);
  ~SignedExeUpdateEngine() override;
  SignedExeUpdateEngine(const SignedExeUpdateEngine&) = delete;
  SignedExeUpdateEngine& operator=(const SignedExeUpdateEngine&) = delete;

  [[nodiscard]] bool supported() const override { return options_.supported; }
  void check(contracts::UpdateEngineHandlers handlers) override;
  bool restart() override;

 private:
  void run(contracts::UpdateEngineHandlers handlers);
  void cleanStaging(const std::string& keepFile) const;
  /** 把一个任务交到界面线程；引擎已销毁时丢弃。 */
  void post(std::function<void()> task);

  Options options_;
  platform::UiThread& ui_;
  contracts::UpdateTransport& transport_;
  std::thread worker_;
  std::atomic<bool> cancel_{false};
  /** 引擎存在期间为 true：投到界面线程的任务执行时引擎已销毁就丢弃。 */
  std::shared_ptr<bool> alive_{std::make_shared<bool>(true)};
  /** 已下载好并通过校验的新程序；只在界面线程读写。 */
  std::filesystem::path ready_;
};

}  // namespace ember::win32::infra
