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

#include <algorithm>
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
/** 开机与复位后遮黑的帧数（约 0.1 秒）。 */
constexpr int kStartupMaskFrames = 6;
/** 不透明的黑色像素（内存顺序 R、G、B、A）。 */
constexpr std::uint32_t kOpaqueBlack = 0xFF000000U;
/** 状态里的速度百分比；速度调节以后再做。 */
constexpr std::int64_t SPEED_PERCENT = 100;

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
  summary.devices = insertion.match.devices;
  for (const auto& item : storage) {
    summary.storage.push_back({item.id, item.kind == hw::StorageKind::Rom ? "rom" : "ram", static_cast<std::int64_t>(item.size), item.battery});
  }
  return summary;
}

}  // namespace

SimulationRuntime::SimulationRuntime(SimulationRuntimeOptions options) : options_(std::move(options)) {
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
    // 正在进行的授权运行以 interrupted 结束，等待它的调用方才不会永远阻塞。
    finishRun("interrupted");
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
  // 帧节拍晚了会让声音缓冲被抽空，所以模拟线程启动时调用选项里的钩子（提高优先级），返回值持有到线程结束。
  const std::shared_ptr<void> threadScope = options_.threadScope ? options_.threadScope() : nullptr;
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
    // 授权运行不限速，跑完一帧马上跑下一帧（命令在循环开头处理）。
    if (run_.has_value()) continue;
    if (framePeriod_.count() <= 0) continue;
    next += framePeriod_;
    const auto now = Clock::now();
    if (now - next > framePeriod_ * MAX_LAG_FRAMES) {
      next = now;
    } else {
      // 休眠中收到命令就处理，处理完接着睡到帧节拍；命令不能让下一帧提前开始，否则连续的输入（如鼠标移动）会让机器与声音加速。
      while (!stopping_ && !resync_ && !run_.has_value() && running() && Clock::now() < next) {
        wake_.wait_until(lock, next, [this] { return stopping_ || !commands_.empty(); });
        runPending(lock);
      }
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
  // 上电与硬复位会把帧号与周期数清零，正在进行的授权运行先结束。
  finishRun("interrupted");
  machine_->powerOn();
  // 上电与硬复位时清空声音缓冲，不让旧的采样接在新的后面。
  if (options_.audio != nullptr) options_.audio->flush();
  frame_ = 0;
  failureReported_ = false;
  running_ = resume;
  resync_ = true;
  maskFramesLeft_ = hideStartupFrames_.load(std::memory_order_relaxed) ? kStartupMaskFrames : 0;
}

void SimulationRuntime::stopMachine() {
  machine_->powerOff();
  running_ = false;
}

bool SimulationRuntime::refreshMachineRom() {
  if (machine_ == nullptr || cartridge_ != nullptr || !machineRomSource_) return false;
  auto latest = machineRomSource_(host_);
  if (!latest || latest->rom == machineRom_) return false;
  // 先装配好新的主机，失败时原主机不变。
  auto machine = options_.machines->createWithRom(host_, latest->rom);
  finishRun("interrupted");
  machine_ = std::move(machine);
  updateFramePeriod();
  machine_->connectFamilyKeyboard(familyKeyboard_);
  if (inputHook_ != nullptr) inputHook_->machineChanged();
  machineRom_ = std::move(latest->rom);
  romLabel_ = latest->label;
  return true;
}

void SimulationRuntime::pushAudio() {
  if (options_.audio == nullptr) return;
  machine_->takeAudio(audioBuffer_);
  // 采样率与声音输出不同时不输出（目前所有主机型号都是 48000 赫兹，不会发生）。
  if (!audioBuffer_.empty() && machine_->audioSampleRate() == options_.audio->sampleRate()) options_.audio->write(audioBuffer_);
}

void SimulationRuntime::updateFramePeriod() {
  const double rate = options_.framesPerSecond < 0 && machine_ != nullptr ? machine_->framesPerSecond() : options_.framesPerSecond;
  framePeriod_ = rate > 0 ? std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(1.0 / rate)) : std::chrono::nanoseconds{0};
}

void SimulationRuntime::syncAudio() {
  if (options_.audio == nullptr) return;
  // 运行中才播放；暂停、断电、出错时淡出到静音，继续时清掉过时的采样再淡入；授权运行不限速，不播放。
  const bool playing = running() && !run_.has_value();
  if (playing == audioPlaying_) return;
  audioPlaying_ = playing;
  options_.audio->setPlaying(playing);
}

void SimulationRuntime::publishState(const char* reason) {
  const auto current = state();
  options_.events->publish(rt::RUNTIME_STATE_CHANGED_EVENT, nlohmann::json(rt::RuntimeStateChangedData{current.power, current.mode, reason}));
}

void SimulationRuntime::publishSnapshot() {
  syncAudio();
  rt::RuntimeStatusResult snapshot;
  const auto current = state();
  snapshot.power = current.power;
  snapshot.mode = current.mode;
  if (machine_ != nullptr) {
    snapshot.machine = host_;
    if (!romLabel_.empty()) snapshot.romLabel = romLabel_;
    snapshot.cycles = static_cast<std::int64_t>(machine_->cpuRegisters().cycles);
  }
  snapshot.cartridge = summary_;
  snapshot.frame = static_cast<std::int64_t>(frame_);
  snapshot.speed = SPEED_PERCENT;
  mouse_ = mouseAvailable();
  snapshot.mouse = mouse_;
  std::lock_guard lock(snapshotMutex_);
  snapshot_ = std::move(snapshot);
}

bool SimulationRuntime::mouseAvailable() const {
  if (machine_ == nullptr) return false;
  if (machine_->hasMouse()) return true;
  if (!summary_.has_value()) return false;
  const auto& devices = summary_->devices;
  return std::find(devices.begin(), devices.end(), "mouse") != devices.end();
}

void SimulationRuntime::reportFailure(const std::string& module, const std::string& code, const std::string& message) {
  finishRun("error");
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
  if (hw::MachineDebugger* debugger = machine_->debugger()) {
    if (const auto hit = debugger->takeBreakpointHit()) {
      handleBreakpoint(*hit);
      return;
    }
  }
  ++frame_;
  if (inputHook_ != nullptr) inputHook_->frameCompleted(*machine_);
  const hw::FrameBuffer picture = machine_->frame();
  if (picture.width > 0 && picture.height > 0) publishPicture(picture);
  if (!run_.has_value()) pushAudio();
  const bool mouseBefore = mouse_;
  publishSnapshot();
  // 运行中机器的输入设备变了（学习机进入 FC 兼容模式后没有鼠标）：通知界面重新读取状态。
  if (mouse_ != mouseBefore) publishState("inputChanged");
  notifyObserver();
  const auto interval = frameEventInterval_.load(std::memory_order_relaxed);
  if (interval > 0 && frame_ % interval == 0) {
    const auto registers = machine_->cpuRegisters();
    options_.events->publish(rt::RUNTIME_FRAME_EVENT, nlohmann::json(rt::RuntimeFrameData{static_cast<std::int64_t>(frame_), static_cast<std::int64_t>(registers.cycles), registers.pc}));
  }
  checkCartridge();
  if (run_.has_value() && --run_->remaining <= 0) {
    finishRun("completed");
    running_ = false;
    publishState("runEnded");
    publishSnapshot();
  }
}

void SimulationRuntime::handleBreakpoint(const hw::BreakpointHit& hit) {
  finishRun("breakpoint");
  running_ = false;
  const hw::CpuRegisters registers = machine_->cpuRegisters();
  rt::RuntimeBreakpointHitData data;
  data.id = hit.id;
  data.kind = hit.kind == hw::BreakpointKind::Execute ? "execute" : (hit.kind == hw::BreakpointKind::Read ? "read" : "write");
  data.address = hit.address;
  if (hit.kind != hw::BreakpointKind::Execute) data.value = hit.value;
  data.pc = registers.pc;
  data.a = registers.a;
  data.x = registers.x;
  data.y = registers.y;
  data.sp = registers.sp;
  data.status = registers.status;
  options_.events->publish(rt::RUNTIME_BREAKPOINT_HIT_EVENT, nlohmann::json(data));
  publishState("breakpoint");
  publishSnapshot();
  notifyObserver();
}

void SimulationRuntime::finishRun(const char* stopped) {
  if (!run_.has_value()) return;
  rt::RunOutcome outcome;
  outcome.frames = run_->total - run_->remaining;
  const auto cycles = machine_ == nullptr ? 0 : machine_->cpuRegisters().cycles;
  outcome.cycles = cycles > run_->startCycles ? static_cast<std::int64_t>(cycles - run_->startCycles) : 0;
  outcome.stopped = stopped;
  const auto promise = run_->promise;
  run_.reset();
  promise->set_value(outcome);
  options_.events->publish(rt::RUNTIME_RUN_FINISHED_EVENT, nlohmann::json(rt::RuntimeRunFinishedData{outcome.frames, outcome.cycles, outcome.stopped}));
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
    refreshMachineRom();
    startMachine(resume);
    publishState("powerOn");
    publishSnapshot();
    return state();
  });
}

rt::RuntimeState SimulationRuntime::powerOff() {
  return submit([this] {
    if (powered()) {
      finishRun("powerOff");
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
      if (options_.audio != nullptr) options_.audio->flush();
      failureReported_ = false;
      maskFramesLeft_ = hideStartupFrames_.load(std::memory_order_relaxed) ? kStartupMaskFrames : 0;
    } else {
      const bool keepRunning = running();
      // 换了新主机时它还没有通电，不用断电。
      if (!refreshMachineRom()) stopMachine();
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
    finishRun("interrupted");
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
    // 继续运行打断授权运行，模式仍然是运行，改按真实帧率。
    if (run_.has_value()) {
      finishRun("interrupted");
      resync_ = true;
      return state();
    }
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
    auto cartridge = options_.mappers->create(insertion.match, insertion.image, machine->cartridgeIrq(), *machine);
    machine->cartridgeSlot().insert(*cartridge);
    auto summary = makeSummary(insertion, cartridge->storageList());

    finishRun("interrupted");
    if (machine_ != nullptr) machine_->cartridgeSlot().eject();
    cartridge_.reset();
    machine_ = std::move(machine);
    cartridge_ = std::move(cartridge);
    updateFramePeriod();
    machine_->connectFamilyKeyboard(familyKeyboard_);
    if (inputHook_ != nullptr) inputHook_->machineChanged();
    summary_ = summary;
    host_ = insertion.match.host;
    romLabel_.clear();
    machineRom_.clear();
    running_ = false;
    failureReported_ = false;
    frame_ = 0;
    if (insertion.powerOn) startMachine(insertion.resume);
    publishState("load");
    publishSnapshot();
    return rt::CartridgeInsertionResult{std::move(summary), state()};
  });
}

rt::RuntimeState SimulationRuntime::bootMachine(rt::MachineBoot boot) {
  return submit([this, boot = std::move(boot)] {
    // 先装配好新的主机，失败时不影响正在运行的旧主机。
    auto machine = options_.machines->createWithRom(boot.machine, boot.rom);
    finishRun("interrupted");
    if (machine_ != nullptr) machine_->cartridgeSlot().eject();
    cartridge_.reset();
    machine_ = std::move(machine);
    updateFramePeriod();
    machine_->connectFamilyKeyboard(familyKeyboard_);
    if (inputHook_ != nullptr) inputHook_->machineChanged();
    summary_.reset();
    host_ = boot.machine;
    romLabel_ = boot.romLabel;
    machineRom_ = boot.rom;
    running_ = false;
    failureReported_ = false;
    frame_ = 0;
    if (boot.powerOn) startMachine(boot.resume);
    publishState("load");
    publishSnapshot();
    return state();
  });
}

bool SimulationRuntime::useMachine(const std::function<void(hw::Machine&)>& action) {
  return submit([this, &action] {
    if (machine_ == nullptr) return false;
    action(*machine_);
    return true;
  });
}

void SimulationRuntime::runOnSimulation(const std::function<void()>& action) {
  submit([&action] {
    action();
    return true;
  });
}

rt::RuntimeState SimulationRuntime::ejectCartridge() {
  return submit([this] {
    if (machine_ != nullptr) {
      finishRun("interrupted");
      machine_->cartridgeSlot().eject();
      cartridge_.reset();
      machine_.reset();
      if (inputHook_ != nullptr) inputHook_->machineChanged();
      summary_.reset();
      host_.clear();
      romLabel_.clear();
      machineRom_.clear();
      running_ = false;
      frame_ = 0;
      failureReported_ = false;
      publishState("eject");
      publishSnapshot();
    }
    return state();
  });
}

void SimulationRuntime::sendInput(hw::InputEvent event) {
  post([this, event = std::move(event)](bool cancelled) {
    if (cancelled || machine_ == nullptr) return;
    if (inputHook_ != nullptr && inputHook_->handle(event, *machine_)) return;
    // 游戏手柄只给手柄绑定，不交给主机。
    if (!std::holds_alternative<hw::GamepadInput>(event)) machine_->input(event);
  });
}

void SimulationRuntime::setInputHook(std::shared_ptr<rt::InputHook> hook) {
  submit([this, hook = std::move(hook)] { inputHook_ = hook; });
}

void SimulationRuntime::setMachineRomSource(rt::MachineRomSource source) {
  submit([this, source = std::move(source)]() mutable { machineRomSource_ = std::move(source); });
}

void SimulationRuntime::setFamilyKeyboard(bool connected) {
  submit([this, connected] {
    if (familyKeyboard_ == connected) return;
    familyKeyboard_ = connected;
    if (machine_ == nullptr) return;
    machine_->connectFamilyKeyboard(connected);
    // 键盘归属变了：钩子里按着的键要松开。
    if (inputHook_ != nullptr) inputHook_->machineChanged();
  });
}

void SimulationRuntime::setHideStartupFrames(bool hide) { hideStartupFrames_.store(hide, std::memory_order_relaxed); }

void SimulationRuntime::publishPicture(const hw::FrameBuffer& picture) {
  if (maskFramesLeft_ <= 0) {
    options_.frames->publish({picture.width, picture.height, frame_, picture.pixels});
    return;
  }
  --maskFramesLeft_;
  blackFrame_.assign(static_cast<std::size_t>(picture.width) * picture.height, kOpaqueBlack);
  options_.frames->publish({picture.width, picture.height, frame_, blackFrame_});
}

void SimulationRuntime::setFrameEventInterval(std::uint32_t frames) { frameEventInterval_.store(frames, std::memory_order_relaxed); }

std::shared_future<rt::RunOutcome> SimulationRuntime::startRun(std::int64_t frames) {
  return submit([this, frames] {
    if (machine_ == nullptr) throw shared::CoreError(rt::ERROR_NO_CARTRIDGE, "还没有装入卡带，无法运行");
    if (!powered()) throw shared::CoreError(rt::ERROR_NOT_POWERED, "主机没有通电，无法运行");
    finishRun("interrupted");
    ActiveRun active;
    active.total = frames;
    active.remaining = frames;
    active.startCycles = machine_->cpuRegisters().cycles;
    active.promise = std::make_shared<std::promise<rt::RunOutcome>>();
    auto future = active.promise->get_future().share();
    run_ = std::move(active);
    const bool wasRunning = running_;
    running_ = true;
    resync_ = true;
    if (!wasRunning) publishState("run");
    publishSnapshot();
    return future;
  });
}

void SimulationRuntime::cancelRun() {
  submit([this] {
    if (!run_.has_value()) return;
    finishRun("interrupted");
    if (running_) {
      running_ = false;
      publishState("pause");
      publishSnapshot();
    }
  });
}

unsigned SimulationRuntime::padPortCount() {
  return submit([this] { return machine_ == nullptr ? 0U : machine_->padPortCount(); });
}

void SimulationRuntime::setPadButtons(unsigned port, std::uint8_t buttons) {
  submit([this, port, buttons] {
    if (machine_ != nullptr && port < machine_->padPortCount()) machine_->setButtons(port, buttons);
  });
}

rt::FrameImage SimulationRuntime::captureFrame() {
  return submit([this] {
    rt::FrameImage image;
    if (machine_ == nullptr) return image;
    const hw::FrameBuffer picture = machine_->frame();
    if (picture.width == 0 || picture.height == 0) return image;
    image.width = picture.width;
    image.height = picture.height;
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(picture.pixels.data());
    image.rgba.assign(bytes, bytes + picture.pixels.size() * sizeof(std::uint32_t));
    return image;
  });
}

rt::StepOutcome SimulationRuntime::step(hw::StepUnit unit, std::int64_t count) {
  return submit([this, unit, count] {
    if (machine_ == nullptr) throw shared::CoreError(rt::ERROR_NO_CARTRIDGE, "还没有装入卡带，无法单步");
    if (!powered()) throw shared::CoreError(rt::ERROR_NOT_POWERED, "主机没有通电，无法单步");
    hw::MachineDebugger* debugger = machine_->debugger();
    if (debugger == nullptr) throw shared::CoreError(rt::ERROR_DEBUG_UNSUPPORTED, "当前主机型号不支持调试");
    finishRun("interrupted");
    running_ = false;
    hw::StepResult done;
    try {
      done = debugger->step(unit, static_cast<std::uint64_t>(count));
    } catch (const shared::CoreError& error) {
      reportFailure("hardware", error.code(), error.what());
      throw;
    }
    frame_ += done.frames;
    if (done.frames > 0) {
      const hw::FrameBuffer picture = machine_->frame();
      if (picture.width > 0 && picture.height > 0) publishPicture(picture);
    }
    publishState("step");
    publishSnapshot();
    checkCartridge();
    notifyObserver();
    rt::StepOutcome outcome;
    outcome.instructions = static_cast<std::int64_t>(done.instructions);
    outcome.cycles = static_cast<std::int64_t>(done.cycles);
    outcome.frames = static_cast<std::int64_t>(done.frames);
    outcome.pc = machine_->cpuRegisters().pc;
    outcome.state = state();
    return outcome;
  });
}

void SimulationRuntime::inspect(const std::function<void(rt::DebugContext&)>& action) {
  submit([this, &action] {
    if (machine_ == nullptr) throw shared::CoreError(rt::ERROR_NO_CARTRIDGE, "还没有装入卡带");
    if (!powered()) throw shared::CoreError(rt::ERROR_NOT_POWERED, "主机没有通电");
    hw::MachineDebugger* debugger = machine_->debugger();
    if (debugger == nullptr) throw shared::CoreError(rt::ERROR_DEBUG_UNSUPPORTED, "当前主机型号不支持调试");
    rt::DebugContext context{*machine_, *debugger, cartridge_.get()};
    action(context);
    notifyObserver();
  });
}

void SimulationRuntime::setObserver(std::function<void(rt::DebugContext&)> observer) {
  submit([this, &observer] { observer_ = std::move(observer); });
}

void SimulationRuntime::notifyObserver() {
  if (!observer_ || machine_ == nullptr || !powered()) return;
  hw::MachineDebugger* debugger = machine_->debugger();
  if (debugger == nullptr) return;
  rt::DebugContext context{*machine_, *debugger, cartridge_.get()};
  try {
    observer_(context);
  } catch (const std::exception& error) {
    options_.errors->write({"runtime", std::string(shared::ERROR_INTERNAL), error.what(), {{"frame", frame_}}});
  }
}

}  // namespace ember::logic::runtime
