/*
 * ------------------------------------------------------------------------
 * 名称：simulation-runtime.hpp
 * 说明：运行服务的实现：一条模拟线程拥有全部硬件状态（主机与卡带），其他线程的请求投递到命令队列，由模拟线程在帧边界执行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：free 模式按真实帧率连续运行（按帧节拍休眠，休眠中收到命令会提前醒来），paused 停止；stepping 与速度调节以后再做。
 *       每帧完成后把帧缓冲交给帧输出，发 runtime.frame（间隔为 0 时不发）；每帧与每条命令之后检查卡带脚本是否出错，
 *       出错就暂停并发 runtime.error。停止时拒绝新命令、结束线程，排队中的命令以 core.unavailable 完成。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>

#include "contracts/hardware/machine.hpp"
#include "contracts/mappers/mapper-cartridge.hpp"
#include "contracts/runtime/machine-factory.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "platform/events/event-sink.hpp"
#include "platform/logging/error-log.hpp"
#include "platform/streams/frame-output.hpp"

namespace ember::logic::runtime {

/** NTSC 经典 FC 的帧率（每秒帧数）。 */
inline constexpr double NTSC_FRAMES_PER_SECOND = 60.0988;

/** 运行服务的依赖与参数。 */
struct SimulationRuntimeOptions {
  std::shared_ptr<contracts::runtime::MachineFactory> machines;
  std::shared_ptr<contracts::mappers::MapperFactory> mappers;
  std::shared_ptr<platform::FrameOutput> frames;
  std::shared_ptr<platform::EventSink> events;
  std::shared_ptr<platform::ErrorLog> errors;
  /** 连续运行时的帧率；0 表示不限速，一直全速运行（测试用）。 */
  double framesPerSecond{NTSC_FRAMES_PER_SECOND};
};

/** 模拟线程与命令队列。 */
class SimulationRuntime final : public contracts::runtime::RuntimeService {
 public:
  /** 创建并启动模拟线程。 */
  explicit SimulationRuntime(SimulationRuntimeOptions options);
  ~SimulationRuntime() override;

  SimulationRuntime(const SimulationRuntime&) = delete;
  SimulationRuntime& operator=(const SimulationRuntime&) = delete;

  /** 停止：拒绝新命令，结束模拟线程并销毁主机与卡带；可重复调用。 */
  void stop();

  [[nodiscard]] contracts::runtime::RuntimeStatusResult status() const override;
  [[nodiscard]] std::optional<contracts::runtime::CartridgeSummary> cartridge() const override;
  contracts::runtime::RuntimeState powerOn(bool resume) override;
  contracts::runtime::RuntimeState powerOff() override;
  contracts::runtime::RuntimeState reset(contracts::runtime::ResetKind kind) override;
  contracts::runtime::RuntimeState pause() override;
  contracts::runtime::RuntimeState resume() override;
  contracts::runtime::CartridgeInsertionResult insertCartridge(contracts::runtime::CartridgeInsertion insertion) override;
  contracts::runtime::RuntimeState ejectCartridge() override;
  void setButtons(unsigned port, std::uint8_t buttons) override;
  void setFrameEventInterval(std::uint32_t frames) override;

 private:
  /** 队列里的命令；cancelled 为 true 表示运行服务已停止，命令不再执行。 */
  using Command = std::function<void(bool cancelled)>;
  using Clock = std::chrono::steady_clock;

  /** 把命令放进队列；已停止时抛出 core.unavailable。 */
  void post(Command command);
  /** 投递一条命令并等它在模拟线程执行完，返回它的结果，命令里的异常原样抛给调用方。 */
  template <typename Function>
  auto submit(Function function) -> std::invoke_result_t<Function&>;

  void threadMain();
  /** 以下方法只在模拟线程调用。 */
  void runPending(std::unique_lock<std::mutex>& lock);
  void runFrame();
  [[nodiscard]] bool powered() const;
  [[nodiscard]] bool running() const { return running_ && powered(); }
  [[nodiscard]] contracts::runtime::RuntimeState state() const;
  void startMachine(bool resume);
  void stopMachine();
  void publishState(const char* reason);
  void publishSnapshot();
  void reportFailure(const std::string& module, const std::string& code, const std::string& message);
  void checkCartridge();
  void applyButtons();

  SimulationRuntimeOptions options_;
  std::chrono::nanoseconds framePeriod_{0};

  std::once_flag stopOnce_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<Command> commands_;
  bool stopping_{false};

  mutable std::mutex snapshotMutex_;
  contracts::runtime::RuntimeStatusResult snapshot_;

  std::atomic<std::uint32_t> frameEventInterval_{0};

  // 以下只由模拟线程访问：硬件状态的唯一拥有者。卡带先于主机销毁。
  std::unique_ptr<contracts::hardware::Machine> machine_;
  std::unique_ptr<contracts::mappers::MapperCartridge> cartridge_;
  std::optional<contracts::runtime::CartridgeSummary> summary_;
  std::string host_;
  bool running_{false};
  /** 要重新对齐帧节拍（刚开始运行，或落后太多）。 */
  bool resync_{true};
  bool failureReported_{false};
  std::uint64_t frame_{0};
  std::array<std::uint8_t, 2> buttons_{};

  /** 必须最后声明：线程在构造函数末尾启动，析构时先于其他成员停止。 */
  std::thread thread_;
};

}  // namespace ember::logic::runtime
