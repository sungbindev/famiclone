/*
 * ------------------------------------------------------------------------
 * 名称：simulation-runtime-test.cpp
 * 说明：运行服务的测试：模拟线程连续运行与帧输出、电源与复位、暂停与继续、运行中换卡带、失败时保持原状、脚本出错暂停、按键输入（经输入钩子）、帧节拍、声音输出、停止时线程干净退出。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用真实的 nrom 插件经 iNES 解析装配卡带；默认不限速（framesPerSecond 为 0），只有节拍测试用真实帧率。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <variant>

#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/hardware/button-input.hpp"
#include "contracts/runtime/input-hook.hpp"
#include "contracts/runtime/runtime-error.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "platform/audio/null-audio-output.hpp"
#include "shared/errors/core-error.hpp"

namespace {

namespace rt = ember::contracts::runtime;
namespace hw = ember::contracts::hardware;
using ember::bootstrap::MachineCatalog;
using ember::logic::runtime::SimulationRuntime;
using ember::logic::runtime::SimulationRuntimeOptions;
using ember::test::FrameCapture;
using ember::test::SyncEventSink;
using ember::test::waitFor;

/** 总是失败的主机型号目录，用来检查装配失败时保持原状。 */
class FailingMachines final : public rt::MachineFactory {
 public:
  std::unique_ptr<hw::Machine> create(const std::string&) const override { throw ember::shared::CoreError(rt::ERROR_HOST_UNSUPPORTED, "没有这个型号"); }
  std::unique_ptr<hw::Machine> createWithBios(const std::string&, std::vector<std::uint8_t>) const override { throw ember::shared::CoreError(rt::ERROR_HOST_UNSUPPORTED, "没有这个型号"); }
};

/** 运行服务加它用到的一切。 */
struct Rig {
  explicit Rig(double framesPerSecond = 0, std::shared_ptr<rt::MachineFactory> machines = std::make_shared<MachineCatalog>()) {
    registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
    registry->installPackage(ember::test::officialPluginPackage("nrom"));
    registry->installPackage(ember::test::failingPluginPackage());
    SimulationRuntimeOptions options;
    options.machines = std::move(machines);
    options.mappers = registry;
    options.frames = frames;
    options.audio = audio;
    options.events = events;
    options.errors = errors;
    options.framesPerSecond = framesPerSecond;
    runtime = std::make_unique<SimulationRuntime>(std::move(options));
  }

  /** 解析并匹配 NES 文件，得到装入请求。 */
  rt::CartridgeInsertion insertion(const std::vector<std::uint8_t>& file, const std::string& source = "test.nes", bool powerOn = true, bool resume = true) const {
    const ember::logic::cartridges::InesCartridgeLoader loader(registry);
    auto plan = loader.plan(file);
    rt::CartridgeInsertion result;
    result.image = std::move(plan.image);
    result.match = std::move(plan.match);
    result.source = source;
    result.powerOn = powerOn;
    result.resume = resume;
    return result;
  }

  /** 等到又收到 count 帧。 */
  bool waitFrames(std::uint64_t count) const {
    const auto target = frames->count() + count;
    return waitFor([&] { return frames->count() >= target; });
  }

  std::shared_ptr<ember::infra::mappers::PluginRegistry> registry;
  std::shared_ptr<FrameCapture> frames = std::make_shared<FrameCapture>();
  std::shared_ptr<ember::platform::NullAudioOutput> audio = std::make_shared<ember::platform::NullAudioOutput>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<ember::test::RecordingErrorLog> errors = std::make_shared<ember::test::RecordingErrorLog>();
  std::unique_ptr<SimulationRuntime> runtime;
};

/** 空转的卡带。 */
std::vector<std::uint8_t> idleRom() {
  ember::test::Assembler main(0x8000);
  main.jmpSelf();
  return ember::test::makeInesFile(main.bytes());
}

/** 调用并取出错误码；没有抛出时返回空串。 */
template <typename Function>
std::string errorCodeOf(Function&& call) {
  try {
    call();
  } catch (const ember::shared::CoreError& error) {
    return error.code();
  }
  return {};
}

/** 键盘输入事件。 */
hw::KeyInput key(const char* code, bool down) { return hw::KeyInput{code, down}; }

/** 把小键盘 2 当作手柄 1 的 A 键的输入钩子，代替 input 模块挂到运行服务上的真钩子；记录被通知的次数。 */
class NumpadHook final : public rt::InputHook {
 public:
  bool handle(const hw::InputEvent& event, hw::Machine& machine) override {
    const auto* keyEvent = std::get_if<hw::KeyInput>(&event);
    if (keyEvent == nullptr || keyEvent->code != "Numpad2") return false;
    machine.setButtons(0, keyEvent->down ? hw::pad_button::A : 0);
    return true;
  }
  void frameCompleted(hw::Machine&) override { ++frames; }
  void machineChanged() override { ++changes; }

  std::atomic<int> frames{0};
  std::atomic<int> changes{0};
};

TEST(SimulationRuntime, StartsPoweredOffWithoutACartridge) {
  Rig rig;
  const auto status = rig.runtime->status();
  EXPECT_EQ(status.power, "off");
  EXPECT_EQ(status.mode, "paused");
  EXPECT_FALSE(status.machine.has_value());
  EXPECT_FALSE(status.cartridge.has_value());
  EXPECT_EQ(status.frame, 0);
  EXPECT_EQ(status.speed, 100);
  EXPECT_FALSE(rig.runtime->cartridge().has_value());

  EXPECT_EQ(errorCodeOf([&] { rig.runtime->powerOn(true); }), rt::ERROR_NO_CARTRIDGE);
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->reset(rt::ResetKind::Soft); }), rt::ERROR_NO_CARTRIDGE);
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->reset(rt::ResetKind::Hard); }), rt::ERROR_NO_CARTRIDGE);
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->pause(); }), rt::ERROR_NOT_POWERED);
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->resume(); }), rt::ERROR_NOT_POWERED);
  EXPECT_EQ(rig.runtime->powerOff().power, "off");
  EXPECT_EQ(rig.runtime->ejectCartridge().power, "off");
  EXPECT_NO_THROW(rig.runtime->sendInput(key("Space", true)));  // 没有主机时输入被丢弃
  EXPECT_EQ(rig.events->count("runtime.stateChanged"), 0);
}

TEST(SimulationRuntime, InsertingACartridgeRunsFramesAndPublishesThem) {
  Rig rig;
  const auto result = rig.runtime->insertCartridge(rig.insertion(idleRom(), "D:/roms/idle.nes"));
  EXPECT_EQ(result.state.power, "on");
  EXPECT_EQ(result.state.mode, "running");
  EXPECT_EQ(result.cartridge.path, "D:/roms/idle.nes");
  EXPECT_EQ(result.cartridge.mapper, 0);
  EXPECT_EQ(result.cartridge.pluginId, "nrom");
  EXPECT_EQ(result.cartridge.host, "fc");
  EXPECT_EQ(result.cartridge.prgSize, 16384);
  EXPECT_EQ(result.cartridge.chrSize, 0);
  EXPECT_EQ(result.cartridge.crc32Prg.size(), 8U);
  ASSERT_EQ(result.cartridge.storage.size(), 3U);
  EXPECT_EQ(result.cartridge.storage[0].id, "prg");
  EXPECT_EQ(result.cartridge.storage[0].kind, "rom");
  EXPECT_EQ(result.cartridge.storage[0].size, 16384);
  EXPECT_EQ(result.cartridge.storage[1].id, "chr");
  EXPECT_EQ(result.cartridge.storage[1].kind, "ram");  // 文件没有 CHR，用后备的 CHR RAM
  EXPECT_EQ(result.cartridge.storage[1].size, 8192);
  EXPECT_EQ(result.cartridge.storage[2].id, "prg-ram");

  ASSERT_TRUE(rig.waitFrames(20));
  EXPECT_EQ(rig.frames->width(), 256U);
  EXPECT_EQ(rig.frames->height(), 240U);
  EXPECT_FALSE(rig.frames->sizeMismatch());
  EXPECT_FALSE(rig.frames->numberGap());
  const auto status = rig.runtime->status();
  EXPECT_EQ(status.power, "on");
  EXPECT_EQ(status.mode, "running");
  EXPECT_EQ(status.machine, "fc");
  ASSERT_TRUE(status.cartridge.has_value());
  EXPECT_EQ(status.cartridge->pluginId, "nrom");
  EXPECT_GT(status.frame, 0);
  EXPECT_GT(status.cycles, 29000);
  EXPECT_EQ(rig.events->last("runtime.stateChanged")["reason"], "load");
}

TEST(SimulationRuntime, PauseStopsTheFramesAndResumeContinuesThem) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(5));

  const auto paused = rig.runtime->pause();
  EXPECT_EQ(paused.power, "on");
  EXPECT_EQ(paused.mode, "paused");
  const auto frozen = rig.frames->count();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(rig.frames->count(), frozen);
  EXPECT_EQ(rig.runtime->status().mode, "paused");
  EXPECT_EQ(rig.runtime->pause().mode, "paused");  // 重复暂停没有效果

  const auto resumed = rig.runtime->resume();
  EXPECT_EQ(resumed.mode, "running");
  EXPECT_TRUE(rig.waitFrames(5));
  EXPECT_EQ(rig.runtime->resume().mode, "running");

  const auto changes = rig.events->all("runtime.stateChanged");
  ASSERT_GE(changes.size(), 3U);
  EXPECT_EQ(changes[1]["reason"], "pause");
  EXPECT_EQ(changes[2]["reason"], "resume");
}

TEST(SimulationRuntime, PowerOffStopsEverythingAndPowerOnStartsAgainFromFrameOne) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(5));

  const auto off = rig.runtime->powerOff();
  EXPECT_EQ(off.power, "off");
  EXPECT_EQ(off.mode, "paused");
  const auto frozen = rig.frames->count();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(rig.frames->count(), frozen);
  EXPECT_TRUE(rig.runtime->cartridge().has_value());  // 卡带仍插着
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->resume(); }), rt::ERROR_NOT_POWERED);
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->reset(rt::ResetKind::Soft); }), rt::ERROR_NOT_POWERED);
  EXPECT_EQ(rig.runtime->powerOff().power, "off");  // 重复断电没有效果

  // 上电默认暂停，不带 resume 时一帧也不会运行。
  const auto on = rig.runtime->powerOn(false);
  EXPECT_EQ(on.power, "on");
  EXPECT_EQ(on.mode, "paused");
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(rig.frames->count(), frozen);
  EXPECT_EQ(rig.runtime->status().frame, 0);

  EXPECT_EQ(rig.runtime->resume().mode, "running");
  ASSERT_TRUE(rig.waitFrames(3));
  EXPECT_FALSE(rig.frames->numberGap());
  EXPECT_EQ(rig.events->last("runtime.stateChanged")["reason"], "resume");

  // 带 resume 上电：直接运行。
  EXPECT_EQ(rig.runtime->powerOff().power, "off");
  EXPECT_EQ(rig.runtime->powerOn(true).mode, "running");
  ASSERT_TRUE(rig.waitFrames(3));
}

TEST(SimulationRuntime, SoftResetKeepsRamAndHardResetClearsIt) {
  Rig rig;
  const auto program = ember::test::makeBootMarkerProgram(0x16, 0x2A);
  rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(program)));
  ASSERT_TRUE(rig.waitFrames(4));
  const auto cold = rig.frames->dominantPixel();

  // 软复位：内部 RAM 保持，程序看到标记，走热启动段，背景色变了；电源与运行模式不变。
  const auto soft = rig.runtime->reset(rt::ResetKind::Soft);
  EXPECT_EQ(soft.power, "on");
  EXPECT_EQ(soft.mode, "running");
  ASSERT_TRUE(rig.waitFrames(4));
  const auto warm = rig.frames->dominantPixel();
  EXPECT_NE(warm, cold);
  EXPECT_EQ(rig.events->last("runtime.stateChanged")["reason"], "reset");

  // 再软复位一次仍是热启动。
  rig.runtime->reset(rt::ResetKind::Soft);
  ASSERT_TRUE(rig.waitFrames(4));
  EXPECT_EQ(rig.frames->dominantPixel(), warm);

  // 硬复位：断电再上电，内部 RAM 清零，回到冷启动的颜色；运行模式保持。
  const auto hard = rig.runtime->reset(rt::ResetKind::Hard);
  EXPECT_EQ(hard.power, "on");
  EXPECT_EQ(hard.mode, "running");
  ASSERT_TRUE(rig.waitFrames(4));
  EXPECT_EQ(rig.frames->dominantPixel(), cold);
  EXPECT_FALSE(rig.frames->numberGap());
}

TEST(SimulationRuntime, HardResetKeepsAPausedMachinePaused) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(3));
  rig.runtime->pause();
  const auto frames = rig.frames->count();
  const auto hard = rig.runtime->reset(rt::ResetKind::Hard);
  EXPECT_EQ(hard.power, "on");
  EXPECT_EQ(hard.mode, "paused");
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(rig.frames->count(), frames);
  EXPECT_EQ(rig.runtime->status().frame, 0);
}

TEST(SimulationRuntime, InsertingWithoutPowerLeavesTheMachineOff) {
  Rig rig;
  const auto result = rig.runtime->insertCartridge(rig.insertion(idleRom(), "a.nes", false, true));
  EXPECT_EQ(result.state.power, "off");
  EXPECT_EQ(result.state.mode, "paused");
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(rig.frames->count(), 0U);
  EXPECT_EQ(rig.runtime->status().machine, "fc");
  EXPECT_EQ(rig.runtime->powerOn(true).mode, "running");
  EXPECT_TRUE(rig.waitFrames(3));

  // 载入但不运行：上电后暂停。
  const auto paused = rig.runtime->insertCartridge(rig.insertion(idleRom(), "b.nes", true, false));
  EXPECT_EQ(paused.state.power, "on");
  EXPECT_EQ(paused.state.mode, "paused");
}

TEST(SimulationRuntime, ReplacingTheCartridgeWhileRunningSwitchesToTheNewOne) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(ember::test::makeBootMarkerProgram(0x16, 0x16)), "first.nes"));
  ASSERT_TRUE(rig.waitFrames(4));
  const auto first = rig.frames->dominantPixel();

  const auto result = rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(ember::test::makeBootMarkerProgram(0x2A, 0x2A)), "second.nes"));
  EXPECT_EQ(result.cartridge.path, "second.nes");
  EXPECT_EQ(result.state.mode, "running");
  ASSERT_TRUE(rig.waitFrames(4));
  EXPECT_NE(rig.frames->dominantPixel(), first);
  const auto status = rig.runtime->status();
  ASSERT_TRUE(status.cartridge.has_value());
  EXPECT_EQ(status.cartridge->path, "second.nes");
  EXPECT_EQ(rig.runtime->cartridge()->path, "second.nes");
  EXPECT_EQ(rig.events->count("runtime.stateChanged"), 2);
}

TEST(SimulationRuntime, AFailedInsertionLeavesTheRunningMachineUntouched) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom(), "good.nes"));
  ASSERT_TRUE(rig.waitFrames(3));

  // 插件已被卸载：装配失败，原来的主机继续运行。
  auto stale = rig.insertion(idleRom(), "stale.nes");
  stale.match.pluginId = "gone";
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->insertCartridge(stale); }), "mappers.notFound");
  EXPECT_EQ(rig.runtime->status().cartridge->path, "good.nes");
  EXPECT_EQ(rig.runtime->status().mode, "running");
  EXPECT_TRUE(rig.waitFrames(3));

  // 主机型号没有实现：同样保持原状。
  Rig other(0, std::make_shared<FailingMachines>());
  EXPECT_EQ(errorCodeOf([&] { other.runtime->insertCartridge(other.insertion(idleRom())); }), rt::ERROR_HOST_UNSUPPORTED);
  EXPECT_FALSE(other.runtime->cartridge().has_value());
  EXPECT_EQ(other.runtime->status().power, "off");
}

TEST(SimulationRuntime, EjectingTearsTheMachineDown) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(3));
  const auto state = rig.runtime->ejectCartridge();
  EXPECT_EQ(state.power, "off");
  EXPECT_EQ(state.mode, "paused");
  const auto status = rig.runtime->status();
  EXPECT_FALSE(status.machine.has_value());
  EXPECT_FALSE(status.cartridge.has_value());
  EXPECT_EQ(status.frame, 0);
  EXPECT_EQ(rig.events->last("runtime.stateChanged")["reason"], "eject");
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->powerOn(true); }), rt::ERROR_NO_CARTRIDGE);
  EXPECT_EQ(rig.runtime->ejectCartridge().power, "off");  // 空槽再取出没有效果
}

TEST(SimulationRuntime, AScriptErrorPausesTheMachineAndReportsIt) {
  Rig rig;
  // mapper 7 的插件在脚本里抛出异常；程序写一次 $8000 就触发。
  rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(ember::test::makeWriteRegisterProgram(), 7), "bad.nes"));
  ASSERT_TRUE(waitFor([&] { return rig.events->count("runtime.error") == 1; }));
  const auto error = rig.events->last("runtime.error");
  EXPECT_EQ(error["module"], "mappers");
  EXPECT_EQ(error["code"], "mappers.scriptError");
  EXPECT_NE(error["message"].get<std::string>().find("test-failing"), std::string::npos);
  ASSERT_TRUE(waitFor([&] { return rig.runtime->status().mode == "paused"; }));
  EXPECT_EQ(rig.runtime->status().power, "on");
  EXPECT_EQ(rig.events->last("runtime.stateChanged")["reason"], "error");
  ASSERT_EQ(rig.errors->entries.size(), 1U);
  EXPECT_EQ(rig.errors->entries[0].code, "mappers.scriptError");

  // 只报告一次：暴露错误后模拟器保持暂停，不再跑帧，也不重复发 runtime.error。
  const auto frozen = rig.frames->count();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(rig.frames->count(), frozen);
  EXPECT_EQ(rig.events->count("runtime.error"), 1);
}

/** 每轮读手柄 1 的 A 键：没按时背景色为 0x16，按下时为 0x2A。 */
std::vector<std::uint8_t> padProbeProgram() {
  auto paint = [](ember::test::Assembler& code, std::uint8_t color) { code.ppuWrite(0x3F00, color).ldaImm(0).staAbs(0x2006).staAbs(0x2006).jmp(0x8000); };
  constexpr std::uint16_t UNPRESSED_ORIGIN = 0x8011;
  ember::test::Assembler unpressed(UNPRESSED_ORIGIN);
  paint(unpressed, 0x16);
  const auto pressedOrigin = static_cast<std::uint16_t>(UNPRESSED_ORIGIN + unpressed.bytes().size());
  ember::test::Assembler head(0x8000);
  head.ldaImm(1).staAbs(0x4016).ldaImm(0).staAbs(0x4016).ldaAbs(0x4016).andImm(1).bne(pressedOrigin);
  ember::test::Assembler pressed(pressedOrigin);
  paint(pressed, 0x2A);
  std::vector<std::uint8_t> program = head.bytes();
  program.insert(program.end(), unpressed.bytes().begin(), unpressed.bytes().end());
  program.insert(program.end(), pressed.bytes().begin(), pressed.bytes().end());
  return program;
}

TEST(SimulationRuntime, InputReachesTheControllerThroughTheInputHook) {
  Rig rig;
  const auto hook = std::make_shared<NumpadHook>();
  rig.runtime->setInputHook(hook);
  rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(padProbeProgram())));
  ASSERT_TRUE(rig.waitFrames(3));
  const auto released = rig.frames->dominantPixel();
  EXPECT_GE(hook->changes.load(), 1) << "装入卡带时通知钩子换了主机";
  EXPECT_GE(hook->frames.load(), 3) << "每完成一帧通知钩子";

  rig.runtime->sendInput(key("Numpad2", true));  // 钩子把小键盘 2 设为手柄 1 的 A
  ASSERT_TRUE(rig.waitFrames(3));
  const auto pressed = rig.frames->dominantPixel();
  EXPECT_NE(pressed, released);

  rig.runtime->sendInput(key("KeyQ", true));  // 钩子不处理的键交给主机，不影响手柄
  ASSERT_TRUE(rig.waitFrames(3));
  EXPECT_EQ(rig.frames->dominantPixel(), pressed);

  rig.runtime->sendInput(key("Numpad2", false));
  ASSERT_TRUE(rig.waitFrames(3));
  EXPECT_EQ(rig.frames->dominantPixel(), released);
}

TEST(SimulationRuntime, KeysDoNotReachTheControllerWithoutAnInputHook) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(padProbeProgram())));
  ASSERT_TRUE(rig.waitFrames(3));
  const auto released = rig.frames->dominantPixel();
  rig.runtime->sendInput(key("Numpad2", true));  // 手柄的按键全由输入钩子产生，卡带脚本不再设定键位
  ASSERT_TRUE(rig.waitFrames(3));
  EXPECT_EQ(rig.frames->dominantPixel(), released);
}

/** 读当前主机的键盘是否归机器；没有主机时返回空。 */
std::optional<bool> keyboardIsMachineInput(SimulationRuntime& runtime) {
  std::optional<bool> result;
  runtime.useMachine([&](hw::Machine& machine) { result = machine.keyboardIsMachineInput(); });
  return result;
}

TEST(SimulationRuntime, TheFamilyKeyboardSettingAppliesToTheCurrentAndTheNextMachines) {
  Rig rig;
  const auto hook = std::make_shared<NumpadHook>();
  rig.runtime->setInputHook(hook);
  rig.runtime->insertCartridge(rig.insertion(idleRom(), "test.nes", false));
  EXPECT_EQ(keyboardIsMachineInput(*rig.runtime), false) << "默认没有接键盘";

  const int changes = hook->changes.load();
  rig.runtime->setFamilyKeyboard(true);
  EXPECT_EQ(keyboardIsMachineInput(*rig.runtime), true) << "对当前主机立即生效";
  EXPECT_GT(hook->changes.load(), changes) << "归属变了，通知钩子松开按着的键";
  rig.runtime->setFamilyKeyboard(true);
  EXPECT_EQ(hook->changes.load(), changes + 1) << "取值没变不再通知";

  rig.runtime->insertCartridge(rig.insertion(idleRom(), "other.nes", false));
  EXPECT_EQ(keyboardIsMachineInput(*rig.runtime), true) << "换卡带后新主机仍按设置接着键盘";
  rig.runtime->setFamilyKeyboard(false);
  EXPECT_EQ(keyboardIsMachineInput(*rig.runtime), false);
}

TEST(SimulationRuntime, InputSentBeforeTheMachineExistsIsDropped) {
  Rig rig;
  rig.runtime->setInputHook(std::make_shared<NumpadHook>());
  rig.runtime->sendInput(key("Numpad2", true));
  rig.runtime->insertCartridge(rig.insertion(ember::test::makeInesFile(padProbeProgram())));
  ASSERT_TRUE(rig.waitFrames(3));
  const auto released = rig.frames->dominantPixel();
  rig.runtime->sendInput(key("Numpad2", true));
  ASSERT_TRUE(rig.waitFrames(3));
  EXPECT_NE(rig.frames->dominantPixel(), released);
}

TEST(SimulationRuntime, FrameEventsArePublishedOnlyWhenEnabled) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(10));
  EXPECT_EQ(rig.events->count("runtime.frame"), 0);

  rig.runtime->setFrameEventInterval(5);
  const auto before = rig.frames->count();
  ASSERT_TRUE(rig.waitFrames(30));
  const auto frames = rig.events->all("runtime.frame");
  ASSERT_FALSE(frames.empty());
  EXPECT_LE(frames.size(), (rig.frames->count() - before) / 5 + 1);
  for (const auto& data : frames) {
    EXPECT_EQ(data["frame"].get<std::int64_t>() % 5, 0);
    EXPECT_GT(data["cycles"].get<std::int64_t>(), 0);
    EXPECT_EQ(data["pc"], 0x8000);
  }
  rig.runtime->setFrameEventInterval(0);
}

TEST(SimulationRuntime, RunsAtTheNtscFrameRateWithoutBusyWaiting) {
  Rig rig(ember::logic::runtime::NTSC_FRAMES_PER_SECOND);
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(2));
  const auto start = std::chrono::steady_clock::now();
  const auto before = rig.frames->count();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  const auto frames = rig.frames->count() - before;
  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  // 1 秒约 60 帧；留出调度抖动的余量。
  EXPECT_GE(frames, 45U) << elapsed;
  EXPECT_LE(frames, 66U) << elapsed;
}

TEST(SimulationRuntime, CommandsAreAnsweredPromptlyWhileTheFrameLoopSleeps) {
  Rig rig(ember::logic::runtime::NTSC_FRAMES_PER_SECOND);
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(2));
  const auto start = std::chrono::steady_clock::now();
  for (int index = 0; index < 20; ++index) {
    rig.runtime->pause();
    rig.runtime->resume();
  }
  // 40 条命令不能每条都等一个帧周期（约 16 毫秒）：休眠会被命令唤醒。
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::milliseconds(400));
}

TEST(SimulationRuntime, StopEndsTheThreadAndRejectsLaterCommands) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(3));
  rig.runtime->stop();
  const auto frames = rig.frames->count();
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->pause(); }), "core.unavailable");
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->powerOn(true); }), "core.unavailable");
  EXPECT_EQ(errorCodeOf([&] { rig.runtime->sendInput(key("Space", true)); }), "core.unavailable");
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  EXPECT_EQ(rig.frames->count(), frames);
  EXPECT_NO_THROW(rig.runtime->stop());  // 可以重复停止
  EXPECT_NO_THROW(static_cast<void>(rig.runtime->status()));
}

TEST(SimulationRuntime, StoppingWhileOtherThreadsAreSendingCommandsNeverHangs) {
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(3));
  std::atomic<bool> done{false};
  std::vector<std::future<int>> workers;
  for (int worker = 0; worker < 4; ++worker) {
    workers.push_back(std::async(std::launch::async, [&] {
      int rejected = 0;
      while (!done.load()) {
        try {
          rig.runtime->pause();
          rig.runtime->resume();
          rig.runtime->sendInput(key("Space", true));
          static_cast<void>(rig.runtime->status());
        } catch (const ember::shared::CoreError& error) {
          if (error.code() == "core.unavailable") {
            ++rejected;
            break;
          }
        }
      }
      return rejected;
    }));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  auto stopped = std::async(std::launch::async, [&] { rig.runtime->stop(); });
  ASSERT_EQ(stopped.wait_for(std::chrono::seconds(10)), std::future_status::ready);
  done = true;
  for (auto& worker : workers) {
    ASSERT_EQ(worker.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_EQ(worker.get(), 1);
  }
}

TEST(SimulationRuntime, DestroyingARunningRuntimeIsClean) {
  auto rig = std::make_unique<Rig>();
  rig->runtime->insertCartridge(rig->insertion(idleRom()));
  ASSERT_TRUE(rig->waitFrames(3));
  rig.reset();
  SUCCEED();
}

TEST(SimulationRuntime, AudioSampleCountMatchesTheFrameCount) {
  // 每帧 29780.67 个 CPU 周期，按 1789772.67 赫兹换成 48000 赫兹的采样，约 798.7 个。
  Rig rig;
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  ASSERT_TRUE(rig.waitFrames(120));
  // 暂停命令在帧边界执行，之后帧数与写入的采样数都不再变化。
  rig.runtime->pause();
  const auto firstFrames = rig.runtime->status().frame;
  const auto firstSamples = rig.audio->stats().writtenSamples;

  rig.runtime->resume();
  ASSERT_TRUE(rig.waitFrames(120));
  rig.runtime->pause();
  const auto frames = rig.runtime->status().frame - firstFrames;
  const auto samples = rig.audio->stats().writtenSamples - firstSamples;
  EXPECT_GE(frames, 120);
  const double expected = static_cast<double>(frames) * 48000.0 / ember::logic::runtime::NTSC_FRAMES_PER_SECOND;
  EXPECT_NEAR(static_cast<double>(samples), expected, 6.0) << "帧数 " << frames;
}

TEST(SimulationRuntime, AudioPlaysOnlyWhileRunningAndIsClearedOnPowerOnAndReset) {
  Rig rig;
  EXPECT_FALSE(rig.audio->playing());
  rig.runtime->insertCartridge(rig.insertion(idleRom()));
  EXPECT_TRUE(rig.audio->playing());

  rig.runtime->pause();
  EXPECT_FALSE(rig.audio->playing());
  const auto flushesBeforeResume = rig.audio->flushCount();
  rig.runtime->resume();
  EXPECT_TRUE(rig.audio->playing());
  // 继续运行时丢弃过时的采样再淡入。
  EXPECT_GT(rig.audio->flushCount(), flushesBeforeResume);

  const auto flushesBeforeSoft = rig.audio->flushCount();
  rig.runtime->reset(rt::ResetKind::Soft);
  EXPECT_GT(rig.audio->flushCount(), flushesBeforeSoft);
  EXPECT_TRUE(rig.audio->playing());

  const auto flushesBeforeHard = rig.audio->flushCount();
  rig.runtime->reset(rt::ResetKind::Hard);
  EXPECT_GT(rig.audio->flushCount(), flushesBeforeHard);
  EXPECT_TRUE(rig.audio->playing());

  rig.runtime->powerOff();
  EXPECT_FALSE(rig.audio->playing());
  const auto flushesBeforePowerOn = rig.audio->flushCount();
  rig.runtime->powerOn(false);
  EXPECT_GT(rig.audio->flushCount(), flushesBeforePowerOn);
  // 上电后默认暂停，不播放。
  EXPECT_FALSE(rig.audio->playing());

  rig.runtime->resume();
  EXPECT_TRUE(rig.audio->playing());
  rig.runtime->ejectCartridge();
  EXPECT_FALSE(rig.audio->playing());
}

TEST(SimulationRuntime, NoAudioOutputIsFine) {
  // 没有声音输出（空指针）时照常运行。
  SimulationRuntimeOptions options;
  options.machines = std::make_shared<MachineCatalog>();
  auto registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
  registry->installPackage(ember::test::officialPluginPackage("nrom"));
  options.mappers = registry;
  auto frames = std::make_shared<FrameCapture>();
  options.frames = frames;
  options.events = std::make_shared<SyncEventSink>();
  options.errors = std::make_shared<ember::test::RecordingErrorLog>();
  options.framesPerSecond = 0;
  SimulationRuntime runtime(std::move(options));
  const ember::logic::cartridges::InesCartridgeLoader loader(registry);
  auto plan = loader.plan(idleRom());
  rt::CartridgeInsertion insertion;
  insertion.image = std::move(plan.image);
  insertion.match = std::move(plan.match);
  insertion.source = "idle.nes";
  runtime.insertCartridge(std::move(insertion));
  ASSERT_TRUE(waitFor([&] { return frames->count() >= 3; }));
  runtime.pause();
  SUCCEED();
}

}  // namespace
