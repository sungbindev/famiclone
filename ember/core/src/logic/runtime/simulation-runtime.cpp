/*
 * ------------------------------------------------------------------------
 * 名称：simulation-runtime.cpp
 * 说明：运行服务的实现：模拟线程、命令队列、电源与复位、暂停与继续、帧输出与状态事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "logic/runtime/simulation-runtime.hpp"

#include <exception>
#include <future>
#include <string>
#include <type_traits>
#include <utility>

#include "contracts/runtime/runtime-error.hpp"
#include "shared/errors/core-error.hpp"

namespace ember::logic::runtime {

namespace hw = contracts::hardware;
namespace rt = contracts::runtime;

namespace {

/** 落后于帧节拍超过这么多帧时放弃追赶，重新对齐（如机器休眠之后）。 */
constexpr int MAX_LAG_FRAMES = 5;
/** 状态里的速度百分比；速度调节以后再做。 */
constexpr std::int64_t SPEED_PERCENT = 100;
/** 手柄口数量。 */
constexpr unsigned BUTTON_PORT_COUNT = 2;

/** 8 位十六进制（大写），与插件管理里的写法一致。 */
std::string hex8(std::uint32_t value) {
  static constexpr char DIGITS[] = "0123456789ABCDEF";
  std::string text(8, '0');
  for (int index = 7; index >= 0; --index, value >>= 4) text[static_cast<std::size_t>(index)] = DIGITS[value & 0xFU];
  return text;
}

/** 卡带描述：文件头、插件与存储区清单。 */
rt::CartridgeSummary makeSummary(const rt::CartridgeInsertion& insertion, const std::vector<contracts::mappers::StorageSummary>& storage) {
  const auto& description = insertion.image.description;
  rt::CartridgeSummary summary;
  summary.path = insertion.source;
  summary.mapper = description.mapper;
  summary.variant = description.variant;
  summary.prgSize = static_cast<std::int64_t>(description.prgSize);
  summary.chrSize = static_cast<std::int64_t>(description.chrSize);
  summary.mirroring = std::string(contracts::mappers::mirroringName(description.mirroring));
  summary.battery = description.battery;
  summary.crc32Prg = hex8(description.crc32Prg);
  summary.crc32All = hex8(description.crc32All);
  summary.pluginId = insertion.match.pluginId;
  summary.pluginName = insertion.match.pluginName;
  summary.pluginVersion = insertion.match.version;
  summary.host = insertion.match.host;
  for (const auto& item : storage) {
    summary.storage.push_back({item.id, item.kind == hw::StorageKind::Rom ? "rom" : "ram", static_cast<std::int64_t>(item.size), item.battery});
  }
  return summary;
}

}  // namespace

SimulationRuntime::SimulationRuntime(SimulationRuntimeOptions options) : options_(std::move(options)) {
  if (options_.framesPerSecond > 0) framePeriod_ = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / options_.framesPerSecond));
  snapshot_.power = "off";
  snapshot_.mode = "paused";
  snapshot_.speed = SPEED_PERCENT;
  thread_ = std::thread([this] { threadMain(); });
}

SimulationRuntime::~SimulationRuntime() { stop(); }

void SimulationRuntime::stop() {
  std::call_once(stopOnce_, [this] {
    {
      std::lock_guard lock(mutex_);
      stopping_ = true;
    }
    wake_.notify_all();
    if (thread_.joinable()) thread_.join();
    // 线程已结束：排队中的命令以取消完成，等待它们的调用方得到 core.unavailable。
    std::deque<Command> rest;
    {
      std::lock_guard lock(mutex_);
      rest.swap(commands_);
    }
    for (auto& command : rest) command(true);
    // 卡带先于主机销毁，卡槽先拔出卡带。
    if (machine_ != nullptr) machine_->cartridgeSlot().eject();
    cartridge_.reset();
    machine_.reset();
  });
}

// ---------------------------------------------------------------- 命令队列

void SimulationRuntime::post(Command command) {
  {
    std::lock_guard lock(mutex_);
    if (stopping_) throw shared::CoreError(shared::ERROR_UNAVAILABLE, "核心正在停止");
    commands_.push_back(std::move(command));
  }
  wake_.notify_one();
}

template <typename Function>
auto SimulationRuntime::submit(Function function) -> std::invoke_result_t<Function&> {
  using Result = std::invoke_result_t<Function&>;
  auto promise = std::make_shared<std::promise<Result>>();
  auto future = promise->get_future();
  post([promise, function = std::move(function)](bool cancelled) mutable {
    if (cancelled) {
      promise->set_exception(std::make_exception_ptr(shared::CoreError(shared::ERROR_UNAVAILABLE, "核心正在停止")));
      return;
    }
    try {
      if constexpr (std::is_void_v<Result>) {
        function();
        promise->set_value();
      } else {
        promise->set_value(function());
      }
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
  });
  return future.get();
}

void SimulationRuntime::runPending(std::unique_lock<std::mutex>& lock) {
  while (!commands_.empty()) {
    auto command = std::move(commands_.front());
    commands_.pop_front();
    lock.unlock();
    command(false);
    checkCartridge();
    lock.lock();
  }
}

void SimulationRuntime::threadMain() {
  std::unique_lock lock(mutex_);
  Clock::time_point next = Clock::now();
  for (;;) {
    runPending(lock);
    if (stopping_) return;
    if (!running()) {
      wake_.wait(lock, [this] { return stopping_ || !commands_.empty(); });
      resync_ = true;
      continue;
    }
    if (resync_) {
      next = Clock::now();
      resync_ = false;
    }
    lock.unlock();
    runFrame();
    lock.lock();
    if (framePeriod_.count() <= 0) continue;
    next += framePeriod_;
    const auto now = Clock::now();
    if (now - next > framePeriod_ * MAX_LAG_FRAMES) {
      next = now;
    } else if (next > now) {
      wake_.wait_until(lock, next, [this] { return stopping_ || !commands_.empty(); });
    }
  }
}

// ---------------------------------------------------------------- 运行（模拟线程）

bool SimulationRuntime::powered() const { return machine_ != nullptr && machine_->isPoweredOn(); }

rt::RuntimeState SimulationRuntime::state() const {
  rt::RuntimeState result;
  result.power = powered() ? "on" : "off";
  result.mode = running() ? "running" : "paused";
  return result;
}

void SimulationRuntime::startMachine(bool resume) {
  machine_->powerOn();
  frame_ = 0;
  failureReported_ = false;
  running_ = resume;
  resync_ = true;
}

void SimulationRuntime::stopMachine() {
  machine_->powerOff();
  running_ = false;
}

void SimulationRuntime::applyButtons() {
  for (unsigned port = 0; port < BUTTON_PORT_COUNT; ++port) machine_->setButtons(port, buttons_[port]);
}

void SimulationRuntime::publishState(const char* reason) {
  const auto current = state();
  options_.events->publish(rt::RUNTIME_STATE_CHANGED_EVENT, nlohmann::json(rt::RuntimeStateChangedData{current.power, current.mode, reason}));
}

void SimulationRuntime::publishSnapshot() {
  rt::RuntimeStatusResult snapshot;
  const auto current = state();
  snapshot.power = current.power;
  snapshot.mode = current.mode;
  if (machine_ != nullptr) {
    snapshot.machine = host_;
    snapshot.cycles = static_cast<std::int64_t>(machine_->cpuRegisters().cycles);
  }
  snapshot.cartridge = summary_;
  snapshot.frame = static_cast<std::int64_t>(frame_);
  snapshot.speed = SPEED_PERCENT;
  std::lock_guard lock(snapshotMutex_);
  snapshot_ = std::move(snapshot);
}

void SimulationRuntime::reportFailure(const std::string& module, const std::string& code, const std::string& message) {
  running_ = false;
  failureReported_ = true;
  options_.errors->write({module, code, message, {{"frame", frame_}}});
  options_.events->publish(rt::RUNTIME_ERROR_EVENT, nlohmann::json(rt::RuntimeErrorData{module, code, message}));
  publishState("error");
  publishSnapshot();
}

void SimulationRuntime::checkCartridge() {
  if (cartridge_ == nullptr || failureReported_) return;
  if (const auto failure = cartridge_->failure()) reportFailure("mappers", failure->code, failure->message);
}

void SimulationRuntime::runFrame() {
  try {
    machine_->runFrame();
  } catch (const shared::CoreError& error) {
    reportFailure("hardware", error.code(), error.what());
    return;
  } catch (const std::exception& error) {
    options_.errors->write({"runtime", std::string(shared::ERROR_INTERNAL), error.what(), {{"frame", frame_}}});
    reportFailure("runtime", std::string(shared::ERROR_INTERNAL), "模拟器内部错误，详情已写入日志");
    return;
  }
  ++frame_;
  const hw::FrameBuffer picture = machine_->frame();
  if (picture.width > 0 && picture.height > 0) options_.frames->publish({picture.width, picture.height, frame_, picture.pixels});
  publishSnapshot();
  const auto interval = frameEventInterval_.load(std::memory_order_relaxed);
  if (interval > 0 && frame_ % interval == 0) {
    const auto registers = machine_->cpuRegisters();
    options_.events->publish(rt::RUNTIME_FRAME_EVENT, nlohmann::json(rt::RuntimeFrameData{static_cast<std::int64_t>(frame_), static_cast<std::int64_t>(registers.cycles), registers.pc}));
  }
  checkCartridge();
}

// ---------------------------------------------------------------- 对外接口

rt::RuntimeStatusResult SimulationRuntime::status() const {
  std::lock_guard lock(snapshotMutex_);
  return snapshot_;
}

std::optional<rt::CartridgeSummary> SimulationRuntime::cartridge() const {
  std::lock_guard lock(snapshotMutex_);
  return snapshot_.cartridge;
}

rt::RuntimeState SimulationRuntime::powerOn(bool resume) {
  return submit([this, resume] {
    if (machine_ == nullptr) throw shared::CoreError(rt::ERROR_NO_CARTRIDGE, "还没有装入卡带，无法上电");
    startMachine(resume);
    publishState("powerOn");
    publishSnapshot();
    return state();
  });
}

rt::RuntimeState SimulationRuntime::powerOff() {
  return submit([this] {
    if (powered()) {
      stopMachine();
      publishState("powerOff");
      publishSnapshot();
    }
    return state();
  });
}

rt::RuntimeState SimulationRuntime::reset(rt::ResetKind kind) {
  return submit([this, kind] {
    if (machine_ == nullptr) throw shared::CoreError(rt::ERROR_NO_CARTRIDGE, "还没有装入卡带，无法复位");
    if (kind == rt::ResetKind::Soft) {
      if (!powered()) throw shared::CoreError(rt::ERROR_NOT_POWERED, "主机没有通电，无法软复位");
      machine_->softReset();
      failureReported_ = false;
    } else {
      const bool keepRunning = running();
      stopMachine();
      startMachine(keepRunning);
    }
    publishState("reset");
    publishSnapshot();
    return state();
  });
}

rt::RuntimeState SimulationRuntime::pause() {
  return submit([this] {
    if (!powered()) throw shared::CoreError(rt::ERROR_NOT_POWERED, "主机没有通电，无法暂停");
    if (running_) {
      running_ = false;
      publishState("pause");
      publishSnapshot();
    }
    return state();
  });
}

rt::RuntimeState SimulationRuntime::resume() {
  return submit([this] {
    if (!powered()) throw shared::CoreError(rt::ERROR_NOT_POWERED, "主机没有通电，无法继续运行");
    if (!running_) {
      running_ = true;
      resync_ = true;
      publishState("resume");
      publishSnapshot();
    }
    return state();
  });
}

rt::CartridgeInsertionResult SimulationRuntime::insertCartridge(rt::CartridgeInsertion insertion) {
  return submit([this, insertion = std::move(insertion)] {
    // 先装配好新的主机与卡带，任何一步失败都不影响正在运行的旧主机。
    auto machine = options_.machines->create(insertion.match.host);
    auto cartridge = options_.mappers->create(insertion.match, insertion.image, machine->cartridgeIrq());
    machine->cartridgeSlot().insert(*cartridge);
    auto summary = makeSummary(insertion, cartridge->storageList());

    if (machine_ != nullptr) machine_->cartridgeSlot().eject();
    cartridge_.reset();
    machine_ = std::move(machine);
    cartridge_ = std::move(cartridge);
    summary_ = summary;
    host_ = insertion.match.host;
    running_ = false;
    failureReported_ = false;
    frame_ = 0;
    applyButtons();
    if (insertion.powerOn) startMachine(insertion.resume);
    publishState("load");
    publishSnapshot();
    return rt::CartridgeInsertionResult{std::move(summary), state()};
  });
}

rt::RuntimeState SimulationRuntime::ejectCartridge() {
  return submit([this] {
    if (machine_ != nullptr) {
      machine_->cartridgeSlot().eject();
      cartridge_.reset();
      machine_.reset();
      summary_.reset();
      host_.clear();
      running_ = false;
      frame_ = 0;
      failureReported_ = false;
      publishState("eject");
      publishSnapshot();
    }
    return state();
  });
}

void SimulationRuntime::setButtons(unsigned port, std::uint8_t buttons) {
  if (port >= BUTTON_PORT_COUNT) throw shared::SchemaError("手柄口只有 2 个，编号为 0 与 1");
  post([this, port, buttons](bool cancelled) {
    if (cancelled) return;
    buttons_[port] = buttons;
    if (machine_ != nullptr) machine_->setButtons(port, buttons);
  });
}

void SimulationRuntime::setFrameEventInterval(std::uint32_t frames) { frameEventInterval_.store(frames, std::memory_order_relaxed); }

}  // namespace ember::logic::runtime
