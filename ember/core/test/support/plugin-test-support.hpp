/*
 * ------------------------------------------------------------------------
 * 名称：plugin-test-support.hpp
 * 说明：mapper 插件测试的公共辅助：读文件、用 miniz 生成 .emap 包、读取官方插件包与测试插件、假的 IRQ 引脚、测试用的主机型号目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：官方插件包由 npm run plugins:pack 打到 build/ember/plugins（core:test 会先打包），路径经编译参数 EMBER_PLUGIN_DIR 传入；
 *       测试插件的源文件在 core/test/data/test-plugins/<名称>/，测试时现场打包，只用于测试，不随程序发布。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <miniz.h>
#include <nlohmann/json.hpp>

#include "bootstrap/machines/common/mapper-circuit-catalog.hpp"
#include "contracts/hardware/button-input.hpp"
#include "contracts/hardware/irq-output.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "infra/mappers/plugin-registry.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"
#include "platform/events/event-sink.hpp"
#include "platform/logging/error-log.hpp"

namespace ember::test {

/** 读取二进制文件，打不开时抛出异常。 */
inline std::vector<std::uint8_t> readBinaryFile(const std::string& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("无法打开文件：" + path);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/** 读取文本文件。 */
inline std::string readTextFile(const std::string& path) {
  const auto bytes = readBinaryFile(path);
  return std::string(bytes.begin(), bytes.end());
}

/** 包里的一个文件：名称与内容。 */
using PackageFile = std::pair<std::string, std::string>;

/** 把文件打成 zip（.emap 的格式）；文件名原样写入，可以故意写不合规的名字。 */
inline std::vector<std::uint8_t> makeEmap(const std::vector<PackageFile>& files) {
  mz_zip_archive zip{};
  if (mz_zip_writer_init_heap(&zip, 0, 0) == MZ_FALSE) throw std::runtime_error("无法创建 zip");
  for (const auto& [name, content] : files) {
    if (mz_zip_writer_add_mem(&zip, name.c_str(), content.data(), content.size(), MZ_DEFAULT_LEVEL) == MZ_FALSE) throw std::runtime_error("无法写入 zip 条目：" + name);
  }
  void* buffer = nullptr;
  std::size_t size = 0;
  if (mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size) == MZ_FALSE) throw std::runtime_error("无法完成 zip");
  std::vector<std::uint8_t> bytes(static_cast<std::uint8_t*>(buffer), static_cast<std::uint8_t*>(buffer) + size);
  mz_zip_writer_end(&zip);
  return bytes;
}

/** 官方插件包的内容：build/ember/plugins/<id>.emap。 */
inline std::vector<std::uint8_t> officialPluginPackage(const std::string& id) { return readBinaryFile(std::string(EMBER_PLUGIN_DIR) + "/" + id + ".emap"); }

/** 测试插件：把 core/test/data/test-plugins/<名称>/ 下的 manifest.json 与（有的话）main.js 打成包。 */
inline std::vector<std::uint8_t> testPluginPackage(const std::string& name) {
  const std::string directory = std::string(EMBER_TEST_DATA_DIR) + "/test-plugins/" + name + "/";
  std::vector<PackageFile> files{{"manifest.json", readTextFile(directory + "manifest.json")}};
  if (std::filesystem::exists(directory + "main.js")) files.emplace_back("main.js", readTextFile(directory + "main.js"));
  return makeEmap(files);
}

/** 假的 IRQ 引脚：记录当前状态与被拉低的次数。 */
class RecordingIrq final : public contracts::hardware::IrqOutput {
 public:
  void set(bool active) override {
    if (active && !active_) ++assertions_;
    active_ = active;
  }

  [[nodiscard]] bool active() const { return active_; }
  [[nodiscard]] int assertions() const { return assertions_; }

 private:
  bool active_{false};
  int assertions_{0};
};

/** 假的磁带数据线：电平由测试设置，记录比较器，翻转经 edge() 通知监听者。 */
class FakeTapeLine final : public contracts::hardware::TapeLine {
 public:
  [[nodiscard]] bool level() override { return level_; }
  void drive(bool) override {}
  void setComparator(const contracts::hardware::TapeComparator& value) override { comparator = value; }
  void setEdgeListener(std::function<void(bool, std::uint32_t)> value) override { listener = std::move(value); }

  /** 测试用：数据线翻转到 level，并在翻转的时刻通知监听者。 */
  void edge(bool level, std::uint32_t samples) {
    level_ = level;
    if (listener) listener(level, samples);
  }
  /** 测试用：只改电平，不通知。 */
  void setLevel(bool level) { level_ = level; }

  contracts::hardware::TapeComparator comparator;
  std::function<void(bool, std::uint32_t)> listener;

 private:
  bool level_{false};
};

/** 假的手柄口：两个口，记录每个口当前的按键掩码与被设置的次数；tape 不为空时主机上有磁带数据线；带一个调度器，CPU 周期等于 12 个主时钟（与 fc 一致）。 */
class RecordingPads final : public contracts::hardware::PadSink {
 public:
  explicit RecordingPads(contracts::hardware::TapeLine* line = nullptr) : tape(line) {}

  [[nodiscard]] unsigned padPortCount() const override { return 2; }
  [[nodiscard]] contracts::hardware::TapeLine* tapeLine() override { return tape; }
  [[nodiscard]] contracts::hardware::Scheduler* cartridgeScheduler() override { return &scheduler; }
  [[nodiscard]] contracts::hardware::ClockDomain cartridgeCpuClock() const override { return contracts::hardware::ClockDomain{CPU_TICKS}; }
  void setButtons(unsigned port, std::uint8_t buttons) override {
    masks.at(port) = buttons;
    ++calls;
  }

  /** 让时间走过若干个 CPU 周期（到期的卡带事件在其中触发）。 */
  void runCycles(std::int64_t cycles) { scheduler.advance(cycles * CPU_TICKS); }

  static constexpr contracts::hardware::MasterTime CPU_TICKS = 12;
  logic::hardware::MasterScheduler scheduler;
  std::array<std::uint8_t, 2> masks{};
  int calls{0};
  contracts::hardware::TapeLine* tape{nullptr};
};

/** 临时目录：构造时创建，析构时连同里面的内容一起删除。 */
class TempDir {
 public:
  TempDir() {
    static int counter = 0;
    path_ = std::filesystem::temp_directory_path() / ("ember-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(++counter));
    std::filesystem::create_directories(path_);
  }
  ~TempDir() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  [[nodiscard]] const std::filesystem::path& path() const { return path_; }

  /** 在目录里写一个文件，返回它的路径。 */
  std::filesystem::path write(const std::string& name, const std::vector<std::uint8_t>& bytes) const {
    const std::filesystem::path file = path_ / name;
    std::filesystem::create_directories(file.parent_path());
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return file;
  }

 private:
  std::filesystem::path path_;
};

/** 记录型错误日志。 */
class RecordingErrorLog final : public platform::ErrorLog {
 public:
  void write(const platform::ErrorLogEntry& entry) noexcept override { entries.push_back(entry); }
  std::vector<platform::ErrorLogEntry> entries;
};

/** 记录型事件发布。 */
class RecordingEventSink final : public platform::EventSink {
 public:
  void publish(std::string_view name, const nlohmann::json& data) override { events.emplace_back(std::string(name), data); }
  std::vector<std::pair<std::string, nlohmann::json>> events;
};

/** 只有 fc 的主机型号目录，不读也不写任何目录。 */
inline infra::mappers::PluginRegistryOptions fcRegistryOptions() {
  infra::mappers::PluginRegistryOptions options;
  options.hosts.hosts = {"fc"};
  options.circuits = std::make_shared<bootstrap::StandardCircuitCatalog>();
  return options;
}

/** 一份最小的合规清单（mapper 号 7，fc，PRG 与 CHR 各一个窗口），测试在它的基础上改字段。 */
inline std::string minimalManifest(const std::string& id = "test-plugin", int mapper = 7) {
  return R"({
  "id": ")" + id + R"(",
  "name": "测试插件",
  "version": "1.0.0",
  "apiVersion": 1,
  "publisher": "测试",
  "host": ["fc"],
  "match": [{ "mapper": )" + std::to_string(mapper) + R"( }],
  "storage": [
    { "id": "prg", "kind": "rom", "from": "file.prg" },
    { "id": "chr", "kind": "rom", "from": "file.chr", "fallback": { "kind": "ram", "size": 8192 } }
  ],
  "windows": [
    { "id": "prg-low", "bus": "cpu", "range": "$8000-$BFFF", "storage": "prg", "size": 16384, "initial": "first", "access": "r" },
    { "id": "prg-high", "bus": "cpu", "range": "$C000-$FFFF", "storage": "prg", "size": 16384, "initial": "last", "access": "r" },
    { "id": "chr", "bus": "ppu", "range": "$0000-$1FFF", "storage": "chr", "size": 8192, "initial": 0, "access": "rw" }
  ]
})";
}

}  // namespace ember::test
