/*
 * ------------------------------------------------------------------------
 * 名称：debug-inspector-test.cpp
 * 说明：检查器与单步的端到端测试：runtime.step 的各单位、内存空间的读写与错误、总线地址解析、cpu 状态的读写、二进制帧返回内存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用最小的 iNES 程序开机（LDA #$42 / STA $10 / LDX #5 / INX / JMP 循环），经核心的请求通道与真实的调试连接发请求。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/debug-test-client.hpp"
#include "../support/runtime-test-support.hpp"
#include "bootstrap/core.hpp"
#include "platform/network/tcp-socket.hpp"

namespace {

using ember::bootstrap::Core;
using ember::test::DebugTestClient;
using nlohmann::json;
namespace ws = ember::platform::websocket;

struct Reply {
  bool ok = false;
  json payload;
};

Reply call(Core& core, const std::string& name, const json& params = json::object()) {
  static int counter = 0;
  std::promise<Reply> promise;
  auto future = promise.get_future();
  core.request("i" + std::to_string(++counter), name, params.dump(), [&promise](bool ok, std::string payload) { promise.set_value({ok, json::parse(payload)}); });
  if (future.wait_for(std::chrono::seconds(20)) != std::future_status::ready) throw std::runtime_error("请求超时");
  return future.get();
}

/** 从 $8000 开始的程序：LDA #$42、STA $10、LDX #5、INX、JMP $8006（循环）。 */
std::vector<std::uint8_t> testProgram() { return {0xA9, 0x42, 0x85, 0x10, 0xA2, 0x05, 0xE8, 0x4C, 0x06, 0x80}; }

class DebugInspectorTest : public testing::Test {
 protected:
  void SetUp() override {
    root_ = std::filesystem::temp_directory_path() / ("ember-debug-inspector-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root_ / "bundled");
    const auto rom = ember::test::makeInesFile(testProgram());
    std::ofstream(root_ / "test.nes", std::ios::binary).write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
    const auto nrom = ember::test::officialPluginPackage("nrom");
    std::ofstream(root_ / "bundled" / "nrom.emap", std::ios::binary).write(reinterpret_cast<const char*>(nrom.data()), static_cast<std::streamsize>(nrom.size()));
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }

  void start(Core& core) {
    const auto data = (root_ / "data").u8string();
    const auto bundled = (root_ / "bundled").u8string();
    const json options{{"userDataPath", std::string(data.begin(), data.end())}, {"appVersion", "1.0.0"}, {"workerCount", 2}, {"bundledPluginsPath", std::string(bundled.begin(), bundled.end())}};
    core.setEventHandler([this](const std::string& name, const std::string& payload) {
      std::lock_guard lock(mutex_);
      events_.push_back({{"name", name}, {"data", json::parse(payload)}});
    });
    static_cast<void>(core.start(options.dump()));
  }

  /** 装入卡带并上电，保持暂停：此时 PC 在 $8000，还没有执行任何指令。 */
  void loadPaused(Core& core) {
    const auto text = (root_ / "test.nes").u8string();
    ASSERT_TRUE(call(core, "cartridges.load", {{"path", std::string(text.begin(), text.end())}, {"powerOn", true}, {"resume", false}}).ok);
  }

  static std::int64_t field(const json& state, const std::string& name) {
    for (const auto& item : state["fields"]) {
      if (item["name"] == name) return item["value"].get<std::int64_t>();
    }
    return -1;
  }

  /** 进程内请求的结果经 JSON 文本往返，二进制变成 {"bytes":[...]}。 */
  static std::vector<std::uint8_t> bytesOf(const json& data) { return data.at("bytes").get<std::vector<std::uint8_t>>(); }

  static json cpuState(Core& core) {
    const auto reply = call(core, "inspector.getState", {{"module", "cpu"}});
    return reply.payload["states"][0];
  }

  [[nodiscard]] std::vector<json> eventsNamed(const std::string& name) {
    std::lock_guard lock(mutex_);
    std::vector<json> found;
    for (const auto& event : events_) {
      if (event["name"] == name) found.push_back(event["data"]);
    }
    return found;
  }

  std::filesystem::path root_;
  std::mutex mutex_;
  std::vector<json> events_;
};

TEST_F(DebugInspectorTest, RefusesWithoutACartridgeOrPower) {
  Core core;
  start(core);
  EXPECT_EQ(call(core, "inspector.listSpaces").payload["code"], "inspector.noCartridge");
  EXPECT_EQ(call(core, "runtime.step", {{"unit", "instruction"}}).payload["code"], "runtime.noCartridge");
  loadPaused(core);
  ASSERT_TRUE(call(core, "runtime.powerOff").ok);
  EXPECT_EQ(call(core, "inspector.getState").payload["code"], "inspector.notPowered");
  EXPECT_EQ(call(core, "runtime.step", {{"unit", "instruction"}}).payload["code"], "runtime.notPowered");
  core.stop();
}

TEST_F(DebugInspectorTest, StepsByInstructionAndTheRegistersFollow) {
  Core core;
  start(core);
  loadPaused(core);
  EXPECT_EQ(field(cpuState(core), "pc"), 0x8000);

  const auto first = call(core, "runtime.step", {{"unit", "instruction"}});
  ASSERT_TRUE(first.ok) << first.payload.dump();
  EXPECT_EQ(first.payload["instructions"], 1);
  EXPECT_EQ(first.payload["cycles"], 2);
  EXPECT_EQ(first.payload["pc"], 0x8002);
  EXPECT_EQ(first.payload["state"]["mode"], "paused");
  EXPECT_EQ(field(cpuState(core), "a"), 0x42);

  const auto more = call(core, "runtime.step", {{"unit", "instruction"}, {"count", 2}});
  EXPECT_EQ(more.payload["instructions"], 2);
  EXPECT_EQ(more.payload["pc"], 0x8006);
  EXPECT_EQ(field(cpuState(core), "x"), 5);
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.stateChanged").empty(); }));
  EXPECT_EQ(eventsNamed("runtime.stateChanged").back()["reason"], "step");
  core.stop();
}

TEST_F(DebugInspectorTest, StepsByCycleScanlineAndFrame) {
  Core core;
  start(core);
  loadPaused(core);
  const auto cycles = call(core, "runtime.step", {{"unit", "cycle"}, {"count", 10}});
  ASSERT_TRUE(cycles.ok);
  EXPECT_GE(cycles.payload["cycles"].get<int>(), 10);
  EXPECT_LE(cycles.payload["cycles"].get<int>(), 16);

  const auto ppuBefore = call(core, "inspector.getState", {{"module", "ppu"}}).payload["states"][0];
  const auto lines = call(core, "runtime.step", {{"unit", "scanline"}, {"count", 3}});
  ASSERT_TRUE(lines.ok);
  const auto ppuAfter = call(core, "inspector.getState", {{"module", "ppu"}}).payload["states"][0];
  EXPECT_EQ((field(ppuAfter, "scanline") - field(ppuBefore, "scanline") + 262) % 262, 3);

  const auto frames = call(core, "runtime.step", {{"unit", "frame"}, {"count", 2}});
  ASSERT_TRUE(frames.ok);
  EXPECT_EQ(frames.payload["frames"], 2);
  EXPECT_EQ(field(call(core, "inspector.getState", {{"module", "ppu"}}).payload["states"][0], "frame"), 2);
  EXPECT_EQ(call(core, "runtime.status").payload["frame"], 2);
  EXPECT_EQ(call(core, "runtime.step", {{"unit", "frame"}, {"count", 0}}).payload["code"], "core.invalidParams");
  core.stop();
}

TEST_F(DebugInspectorTest, StepInterruptsARunAndPausesTheMachine) {
  Core core;
  start(core);
  loadPaused(core);
  ASSERT_TRUE(call(core, "runtime.run", {{"frames", 10000000}}).ok);
  const auto stepped = call(core, "runtime.step", {{"unit", "instruction"}});
  ASSERT_TRUE(stepped.ok);
  EXPECT_EQ(stepped.payload["state"]["mode"], "paused");
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.runFinished").empty(); }));
  EXPECT_EQ(eventsNamed("runtime.runFinished").front()["stopped"], "interrupted");
  core.stop();
}

TEST_F(DebugInspectorTest, ListsTheMemorySpacesIncludingTheCartridgeStorage) {
  Core core;
  start(core);
  loadPaused(core);
  const auto listed = call(core, "inspector.listSpaces");
  ASSERT_TRUE(listed.ok) << listed.payload.dump();
  std::map<std::string, json> byId;
  for (const auto& space : listed.payload["spaces"]) byId[space["id"]] = space;
  for (const char* id : {"cpu", "ppu", "board.ram", "board.nametable", "chip.oam", "chip.palette"}) EXPECT_TRUE(byId.contains(id)) << id;
  EXPECT_EQ(byId["cpu"]["size"], 65536);
  EXPECT_EQ(byId["board.ram"]["size"], 2048);
  ASSERT_TRUE(byId.contains("cart.prg"));
  EXPECT_EQ(byId["cart.prg"]["writable"], false);
  core.stop();
}

TEST_F(DebugInspectorTest, ReadsMemoryWithoutSideEffects) {
  Core core;
  start(core);
  loadPaused(core);
  ASSERT_TRUE(call(core, "runtime.step", {{"unit", "instruction"}, {"count", 2}}).ok);

  const auto ram = call(core, "inspector.readMemory", {{"space", "board.ram"}, {"address", 0x10}, {"length", 1}});
  ASSERT_TRUE(ram.ok) << ram.payload.dump();
  EXPECT_EQ(ram.payload["length"], 1);
  EXPECT_EQ(bytesOf(ram.payload["data"]), (std::vector<std::uint8_t>{0x42}));
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0x10}, {"length", 1}}).payload["data"])[0], 0x42);
  // 内部 RAM 每 2K 镜像一次。
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0x810}, {"length", 1}}).payload["data"])[0], 0x42);

  // 卡带 PRG 的前几个字节就是程序；经 cpu 空间读 $8000 也一样。
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "cart.prg"}, {"address", 0}, {"length", 4}}).payload["data"]), (std::vector<std::uint8_t>{0xA9, 0x42, 0x85, 0x10}));
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0x8000}, {"length", 4}}).payload["data"]), (std::vector<std::uint8_t>{0xA9, 0x42, 0x85, 0x10}));

  // 读 $2002 会清 vblank，但读内存空间不碰寄存器：读出 0，PPU 状态不变。
  ASSERT_TRUE(call(core, "runtime.step", {{"unit", "frame"}}).ok);
  const auto before = field(call(core, "inspector.getState", {{"module", "ppu"}}).payload["states"][0], "vblank");
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0x2002}, {"length", 1}}).payload["data"])[0], 0);
  EXPECT_EQ(field(call(core, "inspector.getState", {{"module", "ppu"}}).payload["states"][0], "vblank"), before);
  core.stop();
}

TEST_F(DebugInspectorTest, ReadAndWriteErrors) {
  Core core;
  start(core);
  loadPaused(core);
  EXPECT_EQ(call(core, "inspector.readMemory", {{"space", "nowhere"}, {"address", 0}, {"length", 1}}).payload["code"], "inspector.unknownSpace");
  EXPECT_EQ(call(core, "inspector.readMemory", {{"space", "cart.nothing"}, {"address", 0}, {"length", 1}}).payload["code"], "inspector.unknownSpace");
  EXPECT_EQ(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0xFFFF}, {"length", 2}}).payload["code"], "inspector.outOfRange");
  EXPECT_EQ(call(core, "inspector.readMemory", {{"space", "board.ram"}, {"address", 0}, {"length", 4096}}).payload["code"], "inspector.outOfRange");
  EXPECT_EQ(call(core, "inspector.writeMemory", {{"space", "cart.prg"}, {"address", 0}, {"bytes", json::array({1})}}).payload["code"], "inspector.readOnly");
  EXPECT_EQ(call(core, "inspector.writeMemory", {{"space", "board.ram"}, {"address", 2047}, {"bytes", json::array({1, 2})}}).payload["code"], "inspector.outOfRange");
  EXPECT_EQ(call(core, "inspector.writeMemory", {{"space", "board.ram"}, {"address", 0}, {"bytes", json::array({256})}}).payload["code"], "core.invalidParams");
  EXPECT_EQ(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0}, {"length", 0}}).payload["code"], "core.invalidParams");
  core.stop();
}

TEST_F(DebugInspectorTest, WritesMemoryInTheSpaces) {
  Core core;
  start(core);
  loadPaused(core);
  const auto written = call(core, "inspector.writeMemory", {{"space", "board.ram"}, {"address", 0x20}, {"bytes", json::array({1, 2, 3})}});
  ASSERT_TRUE(written.ok) << written.payload.dump();
  EXPECT_EQ(written.payload["written"], 3);
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "cpu"}, {"address", 0x20}, {"length", 3}}).payload["data"]), (std::vector<std::uint8_t>{1, 2, 3}));

  // cpu 空间的写经真实的总线：写 $0030 落在内部 RAM。
  ASSERT_TRUE(call(core, "inspector.writeMemory", {{"space", "cpu"}, {"address", 0x30}, {"bytes", json::array({9})}}).ok);
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "board.ram"}, {"address", 0x30}, {"length", 1}}).payload["data"])[0], 9);

  // 芯片里的存储：OAM 与调色板。
  ASSERT_TRUE(call(core, "inspector.writeMemory", {{"space", "chip.oam"}, {"address", 4}, {"bytes", json::array({7, 8})}}).ok);
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "chip.oam"}, {"address", 4}, {"length", 2}}).payload["data"]), (std::vector<std::uint8_t>{7, 8}));
  ASSERT_TRUE(call(core, "inspector.writeMemory", {{"space", "ppu"}, {"address", 0x3F01}, {"bytes", json::array({0x21})}}).ok);
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "chip.palette"}, {"address", 1}, {"length", 1}}).payload["data"])[0], 0x21);
  // $3F10 镜像 $3F00。
  ASSERT_TRUE(call(core, "inspector.writeMemory", {{"space", "ppu"}, {"address", 0x3F10}, {"bytes", json::array({0x0F})}}).ok);
  EXPECT_EQ(bytesOf(call(core, "inspector.readMemory", {{"space", "ppu"}, {"address", 0x3F00}, {"length", 1}}).payload["data"])[0], 0x0F);
  core.stop();
}

TEST_F(DebugInspectorTest, ResolvesBusAddresses) {
  Core core;
  start(core);
  loadPaused(core);
  const auto resolve = [&](const char* space, int address) { return call(core, "inspector.resolve", {{"space", space}, {"address", address}}).payload["target"]; };
  auto target = resolve("cpu", 0x0810);
  EXPECT_EQ(target["kind"], "ram");
  EXPECT_EQ(target["region"], "board.ram");
  EXPECT_EQ(target["offset"], 0x10);
  target = resolve("cpu", 0x2002);
  EXPECT_EQ(target["kind"], "register");
  EXPECT_EQ(target["region"], "chip.ppu");
  EXPECT_EQ(target["offset"], 2);
  target = resolve("cpu", 0x4018);
  EXPECT_EQ(target["kind"], "none");
  EXPECT_FALSE(target.contains("region"));
  target = resolve("cpu", 0x8003);
  EXPECT_EQ(target["kind"], "rom");
  EXPECT_EQ(target["region"], "cart.prg");
  EXPECT_EQ(target["offset"], 3);
  target = resolve("ppu", 0x3F10);
  EXPECT_EQ(target["region"], "chip.palette");
  EXPECT_EQ(target["offset"], 0);
  EXPECT_EQ(resolve("ppu", 0x3F11)["offset"], 17);
  EXPECT_EQ(call(core, "inspector.resolve", {{"space", "oam"}, {"address", 0}}).payload["code"], "core.invalidParams");
  EXPECT_EQ(call(core, "inspector.resolve", {{"space", "cpu"}, {"address", 70000}}).payload["code"], "core.invalidParams");
  core.stop();
}

TEST_F(DebugInspectorTest, ReadsAndWritesTheCpuState) {
  Core core;
  start(core);
  loadPaused(core);
  const auto all = call(core, "inspector.getState");
  ASSERT_TRUE(all.ok) << all.payload.dump();
  std::vector<std::string> modules;
  for (const auto& state : all.payload["states"]) modules.push_back(state["module"]);
  EXPECT_EQ(modules, (std::vector<std::string>{"cpu", "ppu", "mapper"}));
  EXPECT_EQ(call(core, "inspector.getState", {{"module", "nothing"}}).payload["code"], "inspector.invalidState");

  const auto set = call(core, "inspector.setState", {{"module", "cpu"}, {"fields", json::array({{{"name", "pc"}, {"value", 0x8006}}, {{"name", "a"}, {"value", 7}}})}});
  ASSERT_TRUE(set.ok) << set.payload.dump();
  EXPECT_EQ(field(set.payload["state"], "pc"), 0x8006);
  EXPECT_EQ(field(set.payload["state"], "a"), 7);
  // 设置之后接着从新的 PC 执行：INX 把 X 加 1。
  ASSERT_TRUE(call(core, "runtime.step", {{"unit", "instruction"}}).ok);
  EXPECT_EQ(field(cpuState(core), "x"), 1);
  EXPECT_EQ(field(cpuState(core), "a"), 7);

  // 不合格的修改一项也不生效。
  const auto bad = call(core, "inspector.setState", {{"module", "cpu"}, {"fields", json::array({{{"name", "a"}, {"value", 1}}, {{"name", "pc"}, {"value", 70000}}})}});
  EXPECT_EQ(bad.payload["code"], "inspector.invalidState");
  EXPECT_EQ(field(cpuState(core), "a"), 7);
  EXPECT_EQ(call(core, "inspector.setState", {{"module", "cpu"}, {"fields", json::array({{{"name", "cycles"}, {"value", 1}}})}}).payload["code"], "inspector.invalidState");
  EXPECT_EQ(call(core, "inspector.setState", {{"module", "ppu"}, {"fields", json::array({{{"name", "dot"}, {"value", 1}}})}}).payload["code"], "inspector.invalidState");
  EXPECT_EQ(call(core, "inspector.setState", {{"module", "mapper"}, {"fields", json::array({{{"name", "x"}, {"value", 1}}})}}).payload["code"], "inspector.readOnly");
  core.stop();
}

TEST_F(DebugInspectorTest, MemoryTravelsAsABinaryFrameOverTheDebugConnection) {
  Core core;
  start(core);
  loadPaused(core);
  std::string error;
  auto listener = ember::platform::TcpListener::listenLoopback(0, error);
  const auto port = listener->port();
  listener.reset();
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.port"}, {"value", port}}).ok);
  ASSERT_TRUE(call(core, "preferences.set", {{"id", "debug.enabled"}, {"value", true}}).ok);
  std::string token;
  const auto listed = call(core, "preferences.list");
  for (const auto& item : listed.payload["settings"]) {
    if (item["id"] == "debug.token") token = item["value"].get<std::string>();
  }
  auto client = DebugTestClient::connect(port);
  ASSERT_TRUE(client != nullptr);
  ASSERT_TRUE(client->hello(token).contains("result"));

  const json request{{"jsonrpc", "2.0"}, {"id", 9}, {"method", "inspector.readMemory"}, {"params", {{"space", "cart.prg"}, {"address", 0}, {"length", 4}}}};
  ASSERT_TRUE(client->sendText(request.dump()));
  const auto frame = client->receive();
  ASSERT_TRUE(frame.has_value());
  ASSERT_EQ(frame->opcode, ws::Opcode::Binary);
  const std::size_t jsonLength = (static_cast<std::size_t>(frame->payload[0]) << 24) | (static_cast<std::size_t>(frame->payload[1]) << 16) | (static_cast<std::size_t>(frame->payload[2]) << 8) | frame->payload[3];
  const auto header = json::parse(std::string(frame->payload.begin() + 4, frame->payload.begin() + 4 + static_cast<std::ptrdiff_t>(jsonLength)));
  EXPECT_EQ(header["id"], 9);
  EXPECT_EQ(header["result"]["length"], 4);
  EXPECT_EQ(std::vector<std::uint8_t>(frame->payload.begin() + 4 + static_cast<std::ptrdiff_t>(jsonLength), frame->payload.end()), (std::vector<std::uint8_t>{0xA9, 0x42, 0x85, 0x10}));
  core.stop();
}

TEST_F(DebugInspectorTest, ExecuteBreakpointStopsBeforeTheInstructionAndResumeStepsOverIt) {
  Core core;
  start(core);
  loadPaused(core);
  const auto added = call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0x8004}});
  ASSERT_TRUE(added.ok) << added.payload.dump();
  EXPECT_EQ(added.payload["breakpoint"]["id"], 1);
  EXPECT_EQ(added.payload["breakpoint"]["endAddress"], 0x8004);
  ASSERT_TRUE(call(core, "runtime.resume").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.breakpointHit").empty(); }));
  const auto hit = eventsNamed("runtime.breakpointHit").front();
  EXPECT_EQ(hit["kind"], "execute");
  EXPECT_EQ(hit["address"], 0x8004);
  EXPECT_FALSE(hit.contains("value"));
  EXPECT_EQ(hit["pc"], 0x8004);
  EXPECT_EQ(hit["a"], 0x42);
  EXPECT_EQ(call(core, "runtime.status").payload["mode"], "paused");
  const auto listed = call(core, "inspector.listBreakpoints").payload["breakpoints"];
  ASSERT_EQ(listed.size(), 1U);
  EXPECT_EQ(listed[0]["hits"], 1);

  // 继续运行时先越过刚命中的地址；执行断点在循环里 $8006，连续命中两次
  ASSERT_TRUE(call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0x8006}}).ok);
  ASSERT_TRUE(call(core, "runtime.resume").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return eventsNamed("runtime.breakpointHit").size() >= 2; }));
  EXPECT_EQ(eventsNamed("runtime.breakpointHit")[1]["pc"], 0x8006);
  ASSERT_TRUE(call(core, "runtime.resume").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return eventsNamed("runtime.breakpointHit").size() >= 3; }));
  EXPECT_EQ(eventsNamed("runtime.breakpointHit")[2]["pc"], 0x8006);
  EXPECT_EQ(eventsNamed("runtime.breakpointHit")[2]["id"], 2);
  core.stop();
}

TEST_F(DebugInspectorTest, WriteAndReadBreakpointsStopAfterTheInstruction) {
  Core core;
  start(core);
  loadPaused(core);
  ASSERT_TRUE(call(core, "inspector.addBreakpoint", {{"kind", "write"}, {"address", 0x10}, {"endAddress", 0x1F}}).ok);
  ASSERT_TRUE(call(core, "runtime.resume").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.breakpointHit").empty(); }));
  const auto hit = eventsNamed("runtime.breakpointHit").front();
  EXPECT_EQ(hit["kind"], "write");
  EXPECT_EQ(hit["address"], 0x10);
  EXPECT_EQ(hit["value"], 0x42);
  EXPECT_EQ(hit["pc"], 0x8004);

  // 读断点：清掉写断点，对 $8006 的操作码读取（取指也是总线读）停下
  EXPECT_EQ(call(core, "inspector.clearBreakpoints").payload["removed"], 1);
  ASSERT_TRUE(call(core, "inspector.addBreakpoint", {{"kind", "read"}, {"address", 0x8005}}).ok);
  ASSERT_TRUE(call(core, "runtime.resume").ok);
  ASSERT_TRUE(ember::test::waitFor([&] { return eventsNamed("runtime.breakpointHit").size() >= 2; }));
  const auto readHit = eventsNamed("runtime.breakpointHit")[1];
  EXPECT_EQ(readHit["kind"], "read");
  EXPECT_EQ(readHit["address"], 0x8005);
  EXPECT_EQ(readHit["value"], 0x05);
  core.stop();
}

TEST_F(DebugInspectorTest, RunWithWaitReportsBreakpointAndStepIgnoresBreakpoints) {
  Core core;
  start(core);
  loadPaused(core);
  ASSERT_TRUE(call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0x8002}}).ok);
  // 单步忽略断点：第一条 LDA 执行后 PC 到了断点处，再单步仍然能执行
  ASSERT_TRUE(call(core, "runtime.step", {{"unit", "instruction"}}).ok);
  const auto stepped = call(core, "runtime.step", {{"unit", "instruction"}});
  ASSERT_TRUE(stepped.ok);
  EXPECT_EQ(stepped.payload["pc"], 0x8004);
  EXPECT_TRUE(eventsNamed("runtime.breakpointHit").empty());

  ASSERT_TRUE(call(core, "runtime.reset", {{"kind", "soft"}}).ok);
  const auto run = call(core, "runtime.run", {{"frames", 100}, {"wait", true}});
  ASSERT_TRUE(run.ok) << run.payload.dump();
  EXPECT_EQ(run.payload["stopped"], "breakpoint");
  ASSERT_TRUE(ember::test::waitFor([&] { return !eventsNamed("runtime.runFinished").empty(); }));
  EXPECT_EQ(eventsNamed("runtime.runFinished").front()["stopped"], "breakpoint");
  core.stop();
}

TEST_F(DebugInspectorTest, BreakpointErrorsAndRemoval) {
  Core core;
  start(core);
  EXPECT_EQ(call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0x8000}}).payload["code"], "inspector.noCartridge");
  loadPaused(core);
  EXPECT_EQ(call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0x9000}, {"endAddress", 0x8000}}).payload["code"], "inspector.invalidBreakpoint");
  EXPECT_FALSE(call(core, "inspector.addBreakpoint", {{"kind", "jump"}, {"address", 0x8000}}).ok);
  EXPECT_EQ(call(core, "inspector.removeBreakpoint", {{"id", 99}}).payload["code"], "inspector.noBreakpoint");
  for (int index = 0; index < 256; ++index) ASSERT_TRUE(call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0x8000 + index}}).ok);
  EXPECT_EQ(call(core, "inspector.addBreakpoint", {{"kind", "execute"}, {"address", 0xC000}}).payload["code"], "inspector.breakpointLimit");
  ASSERT_TRUE(call(core, "inspector.removeBreakpoint", {{"id", 1}}).ok);
  EXPECT_EQ(call(core, "inspector.listBreakpoints").payload["breakpoints"].size(), 255U);
  EXPECT_EQ(call(core, "inspector.clearBreakpoints").payload["removed"], 255);
  core.stop();
}

}  // namespace
