/*
 * ------------------------------------------------------------------------
 * 名称：real-rom-support.hpp
 * 说明：用真实 ROM 做端到端测试的公共部分：找 reference 目录里的文件、装配运行服务与卡带、敲键、取画面与比较画面的行、把画面存成 PPM。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：ROM 与磁带在仓库的 reference 目录，找不到时用例跳过；设置环境变量 EMBER_REAL_ROM_DUMP 为目录时，把关键时刻的画面存成 PPM 供人工查看。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "../support/runtime-test-support.hpp"
#include "bootstrap/machines/machine-catalog.hpp"
#include "contracts/hardware/button-input.hpp"
#include "logic/runtime/simulation-runtime.hpp"
#include "platform/audio/null-audio-output.hpp"

namespace ember::test {

/** 从当前目录向上找 reference 下的文件；找不到时返回空。 */
inline std::filesystem::path findReference(const std::string& relative) {
  std::filesystem::path directory = std::filesystem::current_path();
  for (int level = 0; level < 8; ++level) {
    const auto path = directory / "reference" / std::filesystem::path(std::u8string(relative.begin(), relative.end()));
    if (std::filesystem::exists(path)) return path;
    if (!directory.has_parent_path() || directory.parent_path() == directory) break;
    directory = directory.parent_path();
  }
  return {};
}

/** 读整个文件；找不到时为空。 */
inline std::vector<std::uint8_t> readReferenceFile(const std::string& relative) {
  const auto path = findReference(relative);
  if (path.empty()) return {};
  std::ifstream stream(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

/** 读 ember/roms 里没有文件头的纯 ROM（如 pec9588.rom）；找不到时为空。 */
inline std::vector<std::uint8_t> readMachineRom(const std::string& fileName) {
  std::filesystem::path directory = std::filesystem::current_path();
  for (int level = 0; level < 8; ++level) {
    const auto path = directory / "ember" / "roms" / fileName;
    if (std::filesystem::exists(path)) {
      std::ifstream stream(path, std::ios::binary);
      return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }
    if (!directory.has_parent_path() || directory.parent_path() == directory) break;
    directory = directory.parent_path();
  }
  return {};
}

/** reference/tapes 里按名字排第一的 m4a；没有时为空。 */
inline std::filesystem::path firstTape() {
  const auto directory = findReference("tapes");
  if (directory.empty()) return {};
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.path().extension() == ".m4a") files.push_back(entry.path());
  }
  std::sort(files.begin(), files.end());
  return files.empty() ? std::filesystem::path() : files.front();
}

/** 存放画面的目录（环境变量 EMBER_REAL_ROM_DUMP）；没有设置时为空。 */
inline std::string dumpDirectory() {
#ifdef _WIN32
  char* value = nullptr;
  std::size_t size = 0;
  if (_dupenv_s(&value, &size, "EMBER_REAL_ROM_DUMP") != 0 || value == nullptr) return {};
  std::string result(value);
  std::free(value);
  return result;
#else
  const char* value = std::getenv("EMBER_REAL_ROM_DUMP");
  return value == nullptr ? std::string() : std::string(value);
#endif
}

/** 画面第 row 行的像素是否相同。 */
inline bool sameRow(const contracts::runtime::FrameImage& first, unsigned firstRow, const contracts::runtime::FrameImage& second, unsigned secondRow) {
  const std::size_t stride = static_cast<std::size_t>(first.width) * 4;
  return first.width == second.width && std::equal(first.rgba.begin() + static_cast<std::ptrdiff_t>(firstRow * stride), first.rgba.begin() + static_cast<std::ptrdiff_t>((firstRow + 1) * stride),
                                                   second.rgba.begin() + static_cast<std::ptrdiff_t>(secondRow * stride));
}

/** 两个画面之间有差异的第一段连续的行。 */
inline std::vector<unsigned> differingRows(const contracts::runtime::FrameImage& before, const contracts::runtime::FrameImage& after) {
  std::vector<unsigned> rows;
  for (unsigned y = 0; y < before.height && y < after.height; ++y) {
    const bool differs = !sameRow(before, y, after, y);
    if (differs) {
      rows.push_back(y);
    } else if (!rows.empty()) {
      break;
    }
  }
  return rows;
}

/** 两个画面是否完全相同。 */
inline bool sameFrame(const contracts::runtime::FrameImage& first, const contracts::runtime::FrameImage& second) { return first.width == second.width && first.height == second.height && first.rgba == second.rgba; }

/** 在 later 画面里，比 original 的 rows 行低 8 的整数倍的位置上，有几处与 original 里的 rows 行完全一样（包括原来的位置，所以至少是 1）。 */
inline unsigned countRepeats(const contracts::runtime::FrameImage& original, const contracts::runtime::FrameImage& later, const std::vector<unsigned>& rows) {
  unsigned count = 0;
  for (unsigned shift = 0; rows.back() + shift < later.height; shift += 8) {
    bool all = true;
    for (const unsigned row : rows) all = all && sameRow(original, row, later, row + shift);
    if (all) ++count;
  }
  return count;
}

/** 运行服务加经典 FC 的装配，装好 nrom 之外指定的官方插件。 */
struct RealRomBench {
  explicit RealRomBench(const std::vector<std::string>& plugins = {"nrom"}) {
    registry = std::make_shared<infra::mappers::PluginRegistry>(fcRegistryOptions());
    for (const std::string& id : plugins) registry->installPackage(officialPluginPackage(id));
    logic::runtime::SimulationRuntimeOptions options;
    options.machines = std::make_shared<bootstrap::MachineCatalog>();
    options.mappers = registry;
    options.frames = frames;
    options.audio = std::make_shared<platform::NullAudioOutput>();
    options.events = events;
    options.errors = errors;
    options.framesPerSecond = 0;
    runtime = std::make_unique<logic::runtime::SimulationRuntime>(std::move(options));
  }

  /** 装入卡带并开机运行；keyboard 为真时接上 Family BASIC 键盘。 */
  void boot(const std::vector<std::uint8_t>& file, bool keyboard = false) {
    const logic::cartridges::InesCartridgeLoader loader(registry);
    auto plan = loader.plan(file);
    contracts::runtime::CartridgeInsertion insertion;
    insertion.image = std::move(plan.image);
    insertion.match = std::move(plan.match);
    insertion.source = "real-rom.nes";
    insertion.powerOn = true;
    insertion.resume = true;
    runtime->setFamilyKeyboard(keyboard);
    runtime->insertCartridge(std::move(insertion));
  }

  /** 按机型标识装入整机机型的 ROM 并开机运行。 */
  void bootMachine(const std::string& machine, std::vector<std::uint8_t> rom) {
    contracts::runtime::MachineBoot boot;
    boot.machine = machine;
    boot.rom = std::move(rom);
    boot.powerOn = true;
    boot.resume = true;
    runtime->bootMachine(std::move(boot));
  }

  /** 再运行 count 帧。 */
  bool run(std::uint64_t count) {
    const auto target = frames->count() + count;
    return waitFor([&] { return frames->count() >= target; });
  }

  /** 按住一个键 hold 帧再松开，松开后留 gap 帧。 */
  void tap(const char* code, std::uint64_t hold = 4, std::uint64_t gap = 4) {
    runtime->sendInput(contracts::hardware::KeyInput{code, true});
    run(hold);
    runtime->sendInput(contracts::hardware::KeyInput{code, false});
    run(gap);
  }

  /** 按住 Shift 敲一个键。 */
  void tapShifted(const char* code) {
    runtime->sendInput(contracts::hardware::KeyInput{"ShiftLeft", true});
    run(2);
    tap(code);
    runtime->sendInput(contracts::hardware::KeyInput{"ShiftLeft", false});
    run(2);
  }

  /** 把当前画面存成 PPM（只在设置了 EMBER_REAL_ROM_DUMP 时）。 */
  void dump(const std::string& name) {
    const std::string directory = dumpDirectory();
    if (directory.empty()) return;
    const auto image = runtime->captureFrame();
    std::ofstream out(std::filesystem::path(directory) / (name + ".ppm"), std::ios::binary);
    out << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    for (std::size_t index = 0; index + 3 < image.rgba.size(); index += 4) out.write(reinterpret_cast<const char*>(&image.rgba[index]), 3);
  }

  std::shared_ptr<infra::mappers::PluginRegistry> registry;
  std::shared_ptr<FrameCapture> frames = std::make_shared<FrameCapture>();
  std::shared_ptr<SyncEventSink> events = std::make_shared<SyncEventSink>();
  std::shared_ptr<RecordingErrorLog> errors = std::make_shared<RecordingErrorLog>();
  std::unique_ptr<logic::runtime::SimulationRuntime> runtime;
};

}  // namespace ember::test
