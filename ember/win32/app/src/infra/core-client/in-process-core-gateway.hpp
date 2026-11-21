/*
 * ------------------------------------------------------------------------
 * 名称：in-process-core-gateway.hpp
 * 说明：进程内的核心网关：外壳进程里直接持有核心（Core 类），不经 Node-API、不经网络；核心在自己的线程里发出的结果、事件与帧都回到界面线程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：核心的启动要读取插件与 ROM，放在后台线程里做，期间状态是 Starting，这时发出的请求排队，就绪后按顺序发给核心，启动失败时以 core.unavailable 失败；
 *       事件与请求结果经 UiThread 投递；帧交给 LatestFrameSlot；必须在界面线程创建与使用（输入消息 sendInput 除外，可在任意线程）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "contracts/core-gateway/core-gateway.hpp"
#include "contracts/logging/error-logger.hpp"
#include "infra/core-client/latest-frame-slot.hpp"
#include "platform/threading/ui-thread.hpp"

namespace ember::bootstrap {
class Core;
}  // namespace ember::bootstrap

namespace ember::win32::infra {

/** 核心网关的选项：对应核心 start 的参数。 */
struct InProcessCoreOptions {
  /** 用户数据目录（UTF-8）。 */
  std::string userDataPath;
  /** 应用版本。 */
  std::string appVersion;
  /** 预装的 mapper 插件目录；没有时为空。 */
  std::optional<std::string> bundledPluginsPath;
  /** 启动参数指定的调试服务端口与令牌。 */
  std::optional<int> debugPort;
  std::optional<std::string> debugToken;
};

/** 进程内的核心网关。 */
class InProcessCoreGateway : public contracts::CoreGateway {
 public:
  InProcessCoreGateway(InProcessCoreOptions options, platform::UiThread& ui, LatestFrameSlot& frames, contracts::ErrorLogger& logger);
  ~InProcessCoreGateway() override;

  void start() override;
  void stop() override;
  [[nodiscard]] contracts::CoreStatus status() const override;
  void request(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done) override;
  void sendInput(const nlohmann::json& message) override;
  [[nodiscard]] std::function<void()> onEvent(contracts::CoreEventHandler handler) override;
  [[nodiscard]] std::function<void()> onStatusChange(contracts::CoreStatusHandler handler) override;

 private:
  struct Shared;
  void setStatus(contracts::CoreStatus status);
  void finishStart(bool ok, const std::string& payload);
  void dispatch(const std::string& name, nlohmann::json params, contracts::CoreReplyHandler done);

  InProcessCoreOptions options_;
  platform::UiThread& ui_;
  LatestFrameSlot& frames_;
  contracts::ErrorLogger& logger_;
  std::shared_ptr<Shared> shared_;
  std::unique_ptr<ember::bootstrap::Core> core_;
  std::thread starter_;
  std::atomic<unsigned long long> nextRequestId_{1};
};

}  // namespace ember::win32::infra
