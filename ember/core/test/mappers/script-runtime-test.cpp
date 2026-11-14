/*
 * ------------------------------------------------------------------------
 * 名称：script-runtime-test.cpp
 * 说明：mapper 脚本沙箱的测试：ctx 宿主对象、参数检查、返回值检查、脚本异常、没有的全局对象、死循环被中止、内存与栈超限、脚本加载错误、跨线程使用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：宿主对象的实现用记录调用的假对象；限制按 docs/03-mapper插件规范.md 第 6.3 节，测试里把时长与内存调小以便快速触发。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <set>
#include <string>
#include <vector>

#include "contracts/mappers/mapper-error.hpp"
#include "infra/mappers/script-runtime.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using namespace ember::infra::mappers;
namespace mapper_error = ember::contracts::mappers;

/** 记录调用的假宿主对象：两个窗口（chr、prg）、两个存储区（rom 只读 4 字节、ram 可写 4 字节）、两项状态（bank、flags）。 */
class FakeBindings final : public ScriptBindings {
 public:
  void windowSetBank(std::size_t window, std::int64_t bank) override {
    if (bank < 0) throw ScriptArgumentError("bank 不能为负数");
    calls.push_back(WINDOWS.at(window) + ".setBank " + std::to_string(bank));
  }
  void windowSetOffset(std::size_t window, std::int64_t offset) override { calls.push_back(WINDOWS.at(window) + ".setOffset " + std::to_string(offset)); }
  void windowUnmap(std::size_t window) override { calls.push_back(WINDOWS.at(window) + ".unmap"); }
  void nametableSet(std::string_view mode) override {
    if (mode != "four" && mode != "horizontal") throw ScriptArgumentError("名称表连法不对");
    calls.push_back("nametable " + std::string(mode));
  }
  void irqAssert() override { calls.emplace_back("irq assert"); }
  void irqRelease() override { calls.emplace_back("irq release"); }
  [[nodiscard]] std::size_t storageSize(std::size_t storage) const override { return storage == 0 ? rom.size() : ram.size(); }
  std::uint8_t storageRead(std::size_t storage, std::int64_t offset) override {
    const auto& bytes = storage == 0 ? rom : ram;
    if (offset < 0 || static_cast<std::size_t>(offset) >= bytes.size()) throw ScriptArgumentError("存储区的偏移越界");
    return bytes[static_cast<std::size_t>(offset)];
  }
  bool storageWrite(std::size_t storage, std::int64_t offset, std::uint8_t value) override {
    if (offset < 0 || static_cast<std::size_t>(offset) >= storageSize(storage)) throw ScriptArgumentError("存储区的偏移越界");
    if (storage == 0) return false;
    ram[static_cast<std::size_t>(offset)] = value;
    return true;
  }
  [[nodiscard]] std::uint32_t stateGet(std::size_t state) const override { return states.at(state); }
  void stateSet(std::size_t state, std::uint32_t value) override { states.at(state) = value; }
  void log(std::string_view message) override { logs.emplace_back(message); }
  [[nodiscard]] bool keyDown(std::string_view code) const override { return held.contains(std::string(code)); }
  void padSet(std::size_t port, std::string_view button, bool down) override {
    if (button != "a" && button != "start") throw ScriptArgumentError("按键不对");
    calls.push_back("pad" + std::to_string(port) + " " + std::string(button) + (down ? " down" : " up"));
  }
  void padRelease(std::size_t port) override { calls.push_back("pad" + std::to_string(port) + " release"); }

  static inline const std::vector<std::string> WINDOWS = {"chr", "prg"};
  std::set<std::string> held;
  std::vector<std::string> calls;
  std::vector<std::string> logs;
  std::vector<std::uint8_t> rom{10, 20, 30, 40};
  std::vector<std::uint8_t> ram{0, 0, 0, 0};
  std::vector<std::uint32_t> states{0, 0};
};

ScriptLayout layout() {
  ScriptLayout result;
  result.windows = {"chr", "prg"};
  result.storages = {"rom", "ram"};
  result.states = {"bank", "flags"};
  result.padPorts = 2;
  result.headerJson = R"({"mapper":7,"variant":1,"prgSize":16384,"chrSize":0,"mirroring":"vertical","battery":false,"trainer":false,"crc32Prg":1,"crc32All":2})";
  return result;
}

/** 限制调小的执行限制：回调 30 毫秒，内存 4 MB。 */
ScriptLimits fastLimits() {
  ScriptLimits limits;
  limits.callTimeout = std::chrono::milliseconds(30);
  limits.memoryBytes = 4U * 1024U * 1024U;
  return limits;
}

/** 执行应当抛出 CoreError，错误码与说明里的片段符合。 */
template <typename Call>
void expectError(Call&& call, std::string_view code, const std::string& fragment) {
  try {
    call();
    FAIL() << "应当抛出异常：" << code;
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), code);
    EXPECT_NE(std::string(error.what()).find(fragment), std::string::npos) << error.what();
  }
}

TEST(ScriptRuntime, CallsResetReadAndWriteWithTheArguments) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      reset(ctx, kind) { ctx.log.warn('reset ' + kind); },
      read(ctx, bus, addr) { return ctx.header.mapper + (bus === 'ppu' ? 100 : 0) + (addr & 1); },
      write(ctx, bus, addr, value) { ctx.log.warn(bus + ':' + addr + ':' + value); }
    };)",
                       layout(), bindings);
  EXPECT_TRUE(runtime.hasReset() && runtime.hasRead() && runtime.hasWrite());
  runtime.reset(true);
  runtime.reset(false);
  EXPECT_EQ(runtime.read(false, 0x8001), 8);
  EXPECT_EQ(runtime.read(true, 0x0000), 107);
  runtime.write(false, 0x8000, 171);
  runtime.write(true, 0x1000, 0);
  EXPECT_EQ(bindings.logs, (std::vector<std::string>{"reset power", "reset soft", "cpu:32768:171", "ppu:4096:0"}));
}

TEST(ScriptRuntime, FunctionsTheScriptDoesNotExportAreSkipped) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", "export default {};", layout(), bindings);
  EXPECT_FALSE(runtime.hasReset() || runtime.hasRead() || runtime.hasWrite());
  EXPECT_FALSE(runtime.hasKey() || runtime.hasMouse() || runtime.hasGamepad());
  runtime.reset(true);
  EXPECT_FALSE(runtime.read(false, 0).has_value());
  runtime.write(false, 0, 0);
  runtime.key(ember::contracts::hardware::KeyInput{"KeyA", true});
  runtime.mouse(ember::contracts::hardware::MouseInput{});
  runtime.gamepad(ember::contracts::hardware::GamepadInput{});
}

TEST(ScriptRuntime, InputCallbacksGetTheEventAndCanUseTheKeysAndPadObjects) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      onKey(ctx, event) {
        if (ctx.keys.isDown('ShiftLeft')) ctx.pad(1).set('start', event.down);
        else ctx.pad(0).set('a', event.down);
        if (event.code === 'KeyR') ctx.pad(0).release();
      },
      onMouse(ctx, event) { ctx.log.warn(event.kind + ' ' + event.x + ' ' + event.buttons); },
      onGamepad(ctx, event) { ctx.log.warn(event.index + ' ' + event.connected + ' ' + event.buttons.length + ' ' + event.axes[0]); }
    };)",
                       layout(), bindings);
  EXPECT_TRUE(runtime.hasKey() && runtime.hasMouse() && runtime.hasGamepad());
  runtime.key(ember::contracts::hardware::KeyInput{"KeyA", true});
  bindings.held.insert("ShiftLeft");
  runtime.key(ember::contracts::hardware::KeyInput{"KeyA", false});
  runtime.key(ember::contracts::hardware::KeyInput{"KeyR", true});
  ember::contracts::hardware::MouseInput mouse;
  mouse.kind = ember::contracts::hardware::MouseKind::Down;
  mouse.x = 12;
  mouse.buttons = 1;
  runtime.mouse(mouse);
  ember::contracts::hardware::GamepadInput pad;
  pad.index = 2;
  pad.connected = true;
  pad.buttons = {true, false, true};
  pad.axes = {0.25};
  runtime.gamepad(pad);
  EXPECT_EQ(bindings.calls, (std::vector<std::string>{"pad0 a down", "pad1 start up", "pad1 start down", "pad0 release"}));
  EXPECT_EQ(bindings.logs, (std::vector<std::string>{"down 12 1", "2 true 3 0.25"}));
}

TEST(ScriptRuntime, PadAndKeyArgumentsAreChecked) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      onKey(ctx, event) {
        if (event.code === 'KeyA') ctx.pad(2);
        if (event.code === 'KeyB') ctx.pad('x');
        if (event.code === 'KeyC') ctx.pad(0).set('turbo', true);
        if (event.code === 'KeyD') ctx.pad(0).set('a', 'yes');
        if (event.code === 'KeyE') ctx.pad(0).set(5, true);
        if (event.code === 'KeyF') ctx.keys.isDown(5);
        if (event.code === 'KeyG') ctx.pad(0).x = 1;
      }
    };)",
                       layout(), bindings);
  for (const char* code : {"KeyA", "KeyB", "KeyC", "KeyD", "KeyE", "KeyF"}) {
    expectError([&] { runtime.key(ember::contracts::hardware::KeyInput{code, true}); }, mapper_error::ERROR_SCRIPT_ERROR, "onKey");
  }
  // 对象是冻结的，模块里给它加属性在严格模式下报 TypeError。
  expectError([&] { runtime.key(ember::contracts::hardware::KeyInput{"KeyG", true}); }, mapper_error::ERROR_SCRIPT_ERROR, "onKey");
  EXPECT_TRUE(bindings.calls.empty());
}

TEST(ScriptRuntime, HeaderIsTheCartridgeDescriptionAndCannotBeChanged) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      read(ctx) {
        const h = ctx.header;
        const same = h.variant === 1 && h.prgSize === 16384 && h.chrSize === 0 && h.mirroring === 'vertical' && h.battery === false && h.crc32All === 2;
        try { h.mapper = 99; } catch (e) { return same && h.mapper === 7 ? 1 : 0; }
        return 0;
      }
    };)",
                       layout(), bindings);
  EXPECT_EQ(runtime.read(false, 0), 1);
}

TEST(ScriptRuntime, WindowOperationsAreForwardedToTheHost) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      write(ctx, bus, addr, value) {
        ctx.window('chr').setBank(value);
        ctx.window('prg').setOffset(addr);
        if (value === 255) ctx.window('chr').unmap();
      }
    };)",
                       layout(), bindings);
  runtime.write(false, 0x4096, 3);
  runtime.write(false, 0, 255);
  EXPECT_EQ(bindings.calls, (std::vector<std::string>{"chr.setBank 3", "prg.setOffset 16534", "chr.setBank 255", "prg.setOffset 0", "chr.unmap"}));
}

TEST(ScriptRuntime, StateStorageIrqNametableAndLogWork) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      write(ctx, bus, addr, value) {
        ctx.state.bank = value;
        ctx.state.flags = 0xFFFFFFFF;
        ctx.log.warn('bank=' + ctx.state.bank + ' flags=' + ctx.state.flags);
        const ram = ctx.storage('ram');
        ram.write(1, 0xAB);
        ctx.log.warn('size=' + ram.size + ' read=' + ram.read(1) + ' romwrite=' + ctx.storage('rom').write(0, 9) + ' ramwrite=' + ram.write(2, 7));
        ctx.log.warn('rom=' + ctx.storage('rom').read(3));
        ctx.irq.assert();
        ctx.irq.release();
        ctx.nametable.set('four');
      }
    };)",
                       layout(), bindings);
  runtime.write(false, 0, 5);
  EXPECT_EQ(bindings.states, (std::vector<std::uint32_t>{5, 0xFFFFFFFFU}));
  EXPECT_EQ(bindings.logs, (std::vector<std::string>{"bank=5 flags=4294967295", "size=4 read=171 romwrite=false ramwrite=true", "rom=40"}));
  EXPECT_EQ(bindings.ram, (std::vector<std::uint8_t>{0, 0xAB, 7, 0}));
  EXPECT_EQ(bindings.rom[0], 10);
  EXPECT_EQ(bindings.calls, (std::vector<std::string>{"irq assert", "irq release", "nametable four"}));
}

TEST(ScriptRuntime, RejectsBadArgumentsToTheHostObjectsAsScriptErrors) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      write(ctx, bus, addr, value) {
        switch (value) {
          case 0: ctx.window('chr').setBank(-1); break;
          case 1: ctx.window('chr').setBank('x'); break;
          case 2: ctx.window('chr').setBank(1.5); break;
          case 3: ctx.window('nothing'); break;
          case 4: ctx.storage('rom').read(99); break;
          case 5: ctx.storage('ram').write(0, 300); break;
          case 6: ctx.nametable.set('diagonal'); break;
          case 7: ctx.state.unknown = 1; break;
          case 8: ctx.window = 5; break;
          case 9: ctx.window(5); break;
          case 10: ctx.log.warn(5); break;
          case 11: ctx.window('chr').setBank(); break;
          case 12: ctx.storage('ram').write(0); break;
          case 13: ctx.nametable.set(1); break;
        }
      }
    };)",
                       layout(), bindings);
  for (std::uint8_t value = 0; value <= 13; ++value) {
    SCOPED_TRACE("case " + std::to_string(value));
    expectError([&] { runtime.write(false, 0, value); }, mapper_error::ERROR_SCRIPT_ERROR, "demo");
  }
  EXPECT_TRUE(bindings.calls.empty());
  EXPECT_EQ(bindings.ram[0], 0);
}

TEST(ScriptRuntime, ReadMustReturnAByteOrUndefined) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      read(ctx, bus, addr) {
        switch (addr) {
          case 0: return undefined;
          case 1: return 0;
          case 2: return 255;
          case 3: return 12.0;
          case 4: return 256;
          case 5: return -1;
          case 6: return 1.5;
          case 7: return 'a';
          case 8: return null;
          case 9: return NaN;
          case 10: return 1n;
          case 11: return {};
        }
      }
    };)",
                       layout(), bindings);
  EXPECT_FALSE(runtime.read(false, 0).has_value());
  EXPECT_EQ(runtime.read(false, 1), 0);
  EXPECT_EQ(runtime.read(false, 2), 255);
  EXPECT_EQ(runtime.read(false, 3), 12);
  for (std::uint16_t address = 4; address <= 11; ++address) {
    SCOPED_TRACE("addr " + std::to_string(address));
    expectError([&] { static_cast<void>(runtime.read(false, address)); }, mapper_error::ERROR_SCRIPT_ERROR, "read 必须返回 0 到 255 的整数或 undefined");
  }
}

TEST(ScriptRuntime, ReportsScriptExceptionsWithThePluginId) {
  FakeBindings bindings;
  ScriptRuntime runtime("my-plugin", R"(
    export default {
      write(ctx, bus, addr, value) {
        if (value === 0) throw new Error('boom');
        if (value === 1) throw 'plain text';
        if (value === 2) null.x;
        if (value === 3) undefinedFunction();
      }
    };)",
                       layout(), bindings);
  expectError([&] { runtime.write(false, 0, 0); }, mapper_error::ERROR_SCRIPT_ERROR, "my-plugin");
  expectError([&] { runtime.write(false, 0, 0); }, mapper_error::ERROR_SCRIPT_ERROR, "Error: boom");
  expectError([&] { runtime.write(false, 0, 1); }, mapper_error::ERROR_SCRIPT_ERROR, "plain text");
  expectError([&] { runtime.write(false, 0, 2); }, mapper_error::ERROR_SCRIPT_ERROR, "TypeError");
  expectError([&] { runtime.write(false, 0, 3); }, mapper_error::ERROR_SCRIPT_ERROR, "ReferenceError: undefinedFunction is not defined");
}

TEST(ScriptRuntime, HasNoAccessToFilesNetworkProcessTimersOrNondeterminism) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", R"(
    export default {
      read(ctx, bus, addr) {
        const absent = typeof process === 'undefined' && typeof require === 'undefined' && typeof fetch === 'undefined' && typeof XMLHttpRequest === 'undefined' &&
          typeof setTimeout === 'undefined' && typeof setInterval === 'undefined' && typeof Date === 'undefined' && typeof performance === 'undefined' &&
          typeof std === 'undefined' && typeof os === 'undefined' && typeof console === 'undefined' && typeof WebSocket === 'undefined' &&
          typeof Math.random === 'undefined';
        // 后续阶段才实现的成员在脚本里是 undefined。
        const later = typeof ctx.timer === 'undefined' && typeof ctx.random === 'undefined' && typeof ctx.settings === 'undefined';
        return absent && later ? 1 : 0;
      },
      write(ctx, bus, addr, value) {
        if (value === 0) process.exit(1);
        if (value === 1) require('fs');
        if (value === 2) new Date();
        if (value === 3) Math.random();
        if (value === 4) setTimeout(() => {}, 1);
        if (value === 5) fetch('http://localhost');
      }
    };)",
                       layout(), bindings);
  EXPECT_EQ(runtime.read(false, 0), 1);
  expectError([&] { runtime.write(false, 0, 0); }, mapper_error::ERROR_SCRIPT_ERROR, "process is not defined");
  expectError([&] { runtime.write(false, 0, 1); }, mapper_error::ERROR_SCRIPT_ERROR, "require is not defined");
  expectError([&] { runtime.write(false, 0, 2); }, mapper_error::ERROR_SCRIPT_ERROR, "Date is not defined");
  expectError([&] { runtime.write(false, 0, 3); }, mapper_error::ERROR_SCRIPT_ERROR, "TypeError");
  expectError([&] { runtime.write(false, 0, 4); }, mapper_error::ERROR_SCRIPT_ERROR, "setTimeout is not defined");
  expectError([&] { runtime.write(false, 0, 5); }, mapper_error::ERROR_SCRIPT_ERROR, "fetch is not defined");
}

TEST(ScriptRuntimeLimits, AnInfiniteLoopIsAbortedAndTheRuntimeStaysUsable) {
  FakeBindings bindings;
  ScriptRuntime runtime("looper", R"(
    export default {
      read() { return 5; },
      write(ctx, bus, addr, value) {
        if (value === 0) for (;;) {}
        // 脚本捕获不了超时的中止。
        try { for (;;) {} } catch (e) { return; }
      }
    };)",
                       layout(), bindings, fastLimits());
  for (const int value : {0, 1}) {
    const auto start = std::chrono::steady_clock::now();
    expectError([&] { runtime.write(false, 0, static_cast<std::uint8_t>(value)); }, mapper_error::ERROR_LIMIT_EXCEEDED, "超过 30 毫秒");
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(5));
  }
  EXPECT_EQ(runtime.read(false, 0), 5);
}

TEST(ScriptRuntimeLimits, ExceedingTheMemoryLimitIsReported) {
  FakeBindings bindings;
  ScriptLimits limits = fastLimits();
  limits.callTimeout = std::chrono::seconds(10);
  ScriptRuntime runtime("hog", R"(
    export default {
      read() { return 1; },
      write(ctx, bus, addr, value) {
        const chunks = [];
        for (;;) chunks.push(new Array(10000).fill(value));
      }
    };)",
                       layout(), bindings, limits);
  expectError([&] { runtime.write(false, 0, 1); }, mapper_error::ERROR_LIMIT_EXCEEDED, "内存");
  EXPECT_EQ(runtime.read(false, 0), 1);
}

TEST(ScriptRuntimeLimits, ExceedingTheStackLimitIsReported) {
  FakeBindings bindings;
  ScriptRuntime runtime("deep", R"(
    function recurse(n) { return recurse(n + 1) + 1; }
    export default { write() { recurse(0); } };)",
                       layout(), bindings, fastLimits());
  expectError([&] { runtime.write(false, 0, 0); }, mapper_error::ERROR_LIMIT_EXCEEDED, "调用栈");
}

TEST(ScriptRuntimeLimits, LoadingAScriptThatNeverFinishesIsAborted) {
  FakeBindings bindings;
  expectError([&] { ScriptRuntime runtime("stuck", "for (;;) {}\nexport default {};", layout(), bindings, fastLimits()); }, mapper_error::ERROR_LIMIT_EXCEEDED, "超过 30 毫秒");
}

TEST(ScriptRuntimeLoadErrors, RejectsScriptsThatCannotBeLoaded) {
  FakeBindings bindings;
  const auto load = [&](const std::string& source) { ScriptRuntime runtime("bad", source, layout(), bindings); };
  expectError([&] { load("export default {"); }, mapper_error::ERROR_INVALID_PACKAGE, "无法");
  expectError([&] { load("export const x = 1;"); }, mapper_error::ERROR_INVALID_PACKAGE, "export default");
  expectError([&] { load("export default 5;"); }, mapper_error::ERROR_INVALID_PACKAGE, "export default");
  expectError([&] { load(""); }, mapper_error::ERROR_INVALID_PACKAGE, "export default");
  expectError([&] { load("import x from 'other'; export default {};"); }, mapper_error::ERROR_INVALID_PACKAGE, "bad");
  expectError([&] { load("export default { read: 5 };"); }, mapper_error::ERROR_INVALID_PACKAGE, "read 必须是函数");
  expectError([&] { load("throw new Error('top level');\nexport default {};"); }, mapper_error::ERROR_INVALID_PACKAGE, "top level");
  expectError([&] { load("await 1;\nexport default {};"); }, mapper_error::ERROR_INVALID_PACKAGE, "await");
}

TEST(ScriptRuntime, ManyRuntimesCanBeCreatedAndDestroyed) {
  FakeBindings bindings;
  for (int index = 0; index < 40; ++index) {
    ScriptRuntime runtime("demo", "export default { read() { return 3; } };", layout(), bindings);
    EXPECT_EQ(runtime.read(false, 0), 3);
  }
}

TEST(ScriptRuntime, ARuntimeCreatedOnOneThreadCanBeUsedFromAnother) {
  FakeBindings bindings;
  ScriptRuntime runtime("demo", "export default { read(ctx, bus, addr) { return addr & 0xFF; } };", layout(), bindings);
  const auto result = std::async(std::launch::async, [&] { return runtime.read(false, 0x1234); }).get();
  EXPECT_EQ(result, 0x34);
}

}  // namespace
