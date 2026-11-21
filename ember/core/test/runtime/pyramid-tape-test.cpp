/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-tape-test.cpp
 * 说明：验证金字塔学习机的磁带：中西文 BASIC 的 SAVE 经 $5100 录进磁带、LOAD 经 $5500 读回来；真实的声像教学磁带驱动菜单 8 的课文画面；立体声磁带的左声道经主机送到扬声器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：真实 ROM 在仓库的 ember/roms，真实磁带在 reference/tapes（m4a，由 FFmpeg 库解码），缺它们时用例跳过；
 *       SAVE 与 LOAD 各要约 76 秒的磁带时间（模拟时间，不是真实时间），所以分段运行，避开单次等待 10 秒的限制；
 *       BASIC 的程序放在 $7001 起（链接指针、行号、记号化的正文），用它比较 LOAD 的结果。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "../support/real-rom-support.hpp"
#include "bootstrap/machines/pyramid/pyramid.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "infra/media/disk-tape-store.hpp"
#include "logic/hardware/tape-deck/tape-deck.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::test::firstTape;
using ember::test::readMachineRom;
using ember::test::RealRomBench;

/** BASIC 程序在 RAM 里的比较长度。 */
constexpr std::size_t PROGRAM_BYTES = 0x20;
/** 第 10 行进入 RAM 的位置：9588 的 BASIC 把程序放在 $7000 起，586 的放在 $6000 起。 */
constexpr std::uint16_t PEC9588_PROGRAM_START = 0x7000;
constexpr std::uint16_t PEC586_PROGRAM_START = 0x6000;
/** SAVE 与 LOAD 各自需要的帧数（约 76 秒，留出余量）。 */
constexpr std::uint64_t TAPE_FRAMES = 5400;
/** 分段运行的帧数：每段不超过测试支持里单次等待的 10 秒。 */
constexpr std::uint64_t CHUNK_FRAMES = 450;

/** 内存里的录音去向：32 kHz 单声道。 */
class MemorySink final : public hw::TapeSink {
 public:
  explicit MemorySink(std::shared_ptr<std::vector<std::int16_t>> samples) : samples_(std::move(samples)) {}
  void write(const std::int16_t* samples, std::size_t count) override { samples_->insert(samples_->end(), samples, samples + count); }
  void finish() override {}

 private:
  std::shared_ptr<std::vector<std::int16_t>> samples_;
};

/** 把 32 kHz 单声道录音按 96 kHz 左右交错重放的来源。 */
class ReplaySource final : public hw::TapeSource {
 public:
  explicit ReplaySource(std::vector<std::int16_t> samples) : samples_(std::move(samples)) {}
  std::size_t read(std::int16_t* interleaved, std::size_t frames) override {
    std::size_t produced = 0;
    while (produced < frames && position_ / 3 < samples_.size()) {
      const std::int16_t value = samples_[position_ / 3];
      interleaved[produced * 2] = value;
      interleaved[produced * 2 + 1] = value;
      ++produced;
      ++position_;
    }
    return produced;
  }
  void rewind() override { position_ = 0; }
  [[nodiscard]] double durationSeconds() const override { return static_cast<double>(samples_.size()) / 32000.0; }
  [[nodiscard]] bool stereo() const override { return false; }

 private:
  std::vector<std::int16_t> samples_;
  std::size_t position_{0};
};

/** 左声道恒为 left、右声道（数据线）恒为 0 的立体声磁带。 */
class LeftToneSource final : public hw::TapeSource {
 public:
  LeftToneSource(std::int16_t left, std::size_t frames) : left_(left), frames_(frames) {}
  std::size_t read(std::int16_t* interleaved, std::size_t frames) override {
    std::size_t produced = 0;
    while (produced < frames && cursor_ < frames_) {
      interleaved[produced * 2] = left_;
      interleaved[produced * 2 + 1] = 0;
      ++produced;
      ++cursor_;
    }
    return produced;
  }
  void rewind() override { cursor_ = 0; }
  [[nodiscard]] double durationSeconds() const override { return static_cast<double>(frames_) / SAMPLE_RATE; }
  [[nodiscard]] bool stereo() const override { return true; }

 private:
  std::int16_t left_;
  std::size_t frames_;
  std::size_t cursor_{0};
};

/** 分段运行 frames 帧；任何一段没跑完就返回 false。 */
bool runLong(RealRomBench& bench, std::uint64_t frames) {
  for (std::uint64_t done = 0; done < frames; done += CHUNK_FRAMES) {
    if (!bench.run(std::min(CHUNK_FRAMES, frames - done))) return false;
  }
  return true;
}

/** 在 BASIC 的提示符下键入文字：只认大写字母、数字、空格与双引号，'\n' 是回车。 */
void typeText(RealRomBench& bench, std::string_view text) {
  for (const char ch : text) {
    if (ch == '\n') {
      bench.tap("Enter", 6, 20);
    } else if (ch == ' ') {
      bench.tap("Space", 6, 6);
    } else if (ch == '"') {
      bench.tapShifted("Quote");
    } else if (ch >= '0' && ch <= '9') {
      const char code[] = {'D', 'i', 'g', 'i', 't', ch, '\0'};
      bench.tap(code, 6, 6);
    } else {
      const char code[] = {'K', 'e', 'y', ch, '\0'};
      bench.tap(code, 6, 6);
    }
  }
}

/** 读出 BASIC 程序所在的内存。 */
std::vector<std::uint8_t> programBytes(RealRomBench& bench, std::uint16_t start) {
  std::vector<std::uint8_t> bytes(PROGRAM_BYTES);
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.debugger()->readMemory("cpu", start, std::span<std::uint8_t>(bytes)); });
  return bytes;
}

/** 开机，离开开机画面（firstKey），从主菜单进入指定的项（按键名如 "Digit5"）。 */
void enterMenuItem(RealRomBench& bench, const char* machine, const std::vector<std::uint8_t>& rom, const char* firstKey, const char* key, std::uint64_t settle) {
  bench.bootMachine(machine, rom);
  ASSERT_TRUE(bench.run(300));
  bench.tap(firstKey, 8, 420);
  bench.tap(key, 8, settle);
}

/** 在 BASIC 里键入第 10 行，SAVE 到磁带、NEW、再 LOAD，程序应与 SAVE 之前一字节不差。 */
void saveAndLoadARoundTrip(RealRomBench& bench, std::uint16_t start, std::string_view saveCommand, std::string_view loadCommand) {
  const auto empty = programBytes(bench, start);
  typeText(bench, "10 PRINT 7\n");
  ASSERT_TRUE(bench.run(60));
  const auto typed = programBytes(bench, start);
  ASSERT_NE(typed, empty) << "键入的第 10 行应该进入程序区";
  // 程序占用的字节数：最后一个与键入之前不同的字节的位置加 1。
  std::size_t length = 0;
  for (std::size_t index = 0; index < typed.size(); ++index) {
    if (typed[index] != empty[index]) length = index + 1;
  }
  const auto programPart = [&](const std::vector<std::uint8_t>& bytes) { return std::vector<std::uint8_t>(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(length)); };

  auto recorded = std::make_shared<std::vector<std::int16_t>>();
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->record(std::make_unique<MemorySink>(recorded), "save"); });
  typeText(bench, saveCommand);
  ASSERT_TRUE(runLong(bench, TAPE_FRAMES));
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->stop(); });
  EXPECT_GT(recorded->size(), 32000U * 30) << "SAVE 应该录进了几十秒的信号";

  typeText(bench, "NEW\n");
  ASSERT_TRUE(bench.run(120));
  // NEW 只重置指针，不一定清掉程序区；这里把程序区清零，LOAD 之后读回的内容才能证明来自磁带。
  bench.runtime->useMachine([&](hw::Machine& machine) {
    const std::vector<std::uint8_t> zeros(length, 0);
    machine.debugger()->writeMemory("cpu", start, zeros);
  });
  EXPECT_NE(programPart(programBytes(bench, start)), programPart(typed)) << "清零之后程序区应该与键入的程序不同";

  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->load(std::make_unique<ReplaySource>(*recorded), "load"); });
  typeText(bench, loadCommand);
  ASSERT_TRUE(bench.run(60));
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->play(); });
  ASSERT_TRUE(runLong(bench, TAPE_FRAMES));
  EXPECT_EQ(programPart(programBytes(bench, start)), programPart(typed)) << "LOAD 读回的程序应该与 SAVE 之前一字节不差";
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(PyramidTape, Pec9588BasicSaveRecordsTheProgramOnTapeAndLoadReadsItBack) {
  const auto rom = readMachineRom("pec9588.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec9588.bin";
  RealRomBench bench;
  ASSERT_NO_FATAL_FAILURE(enterMenuItem(bench, "pec9588", rom, "Enter", "Digit5", 400));
  saveAndLoadARoundTrip(bench, PEC9588_PROGRAM_START, "SAVE\n", "LOAD\n");
}

TEST(PyramidTape, Pec586BasicSaveRecordsTheProgramOnTapeAndLoadReadsItBack) {
  const auto rom = readMachineRom("pec586.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec586.bin";
  RealRomBench bench;
  ASSERT_NO_FATAL_FAILURE(enterMenuItem(bench, "pec586", rom, "Space", "Digit8", 400));
  saveAndLoadARoundTrip(bench, PEC586_PROGRAM_START, "SAVE \"A\"\n", "LOAD \"A\"\n");
}

/** 两个画面里颜色不同的像素个数。 */
std::size_t differingPixels(const ember::contracts::runtime::FrameImage& first, const ember::contracts::runtime::FrameImage& second) {
  std::size_t total = 0;
  for (std::size_t index = 0; index + 3 < first.rgba.size() && index + 3 < second.rgba.size(); index += 4) {
    if (!std::equal(first.rgba.begin() + static_cast<std::ptrdiff_t>(index), first.rgba.begin() + static_cast<std::ptrdiff_t>(index) + 3, second.rgba.begin() + static_cast<std::ptrdiff_t>(index))) ++total;
  }
  return total;
}

/** 进入“声像电视教学”菜单项，放真实的声像磁带，程序经数据线读到课文后画面应该从标志换成课文（playFrames 是放带的帧数）。 */
void lessonTapeDrivesTheLessonScreen(const char* machineId, const char* romFile, const char* firstKey, const char* key, const char* dumpName, std::uint64_t playFrames) {
  const auto rom = readMachineRom(romFile);
  const auto tape = firstTape();
  if (rom.empty() || tape.empty()) GTEST_SKIP() << "没有找到 ember/roms/" << romFile << " 或声像磁带";
  RealRomBench bench;
  ASSERT_NO_FATAL_FAILURE(enterMenuItem(bench, machineId, rom, firstKey, key, 300));
  bench.dump(std::string(dumpName) + "-logo");
  const auto logo = bench.runtime->captureFrame();

  ember::infra::media::DiskTapeStore store(nullptr);
  const auto tapePath = tape.u8string();
  auto source = store.open(std::string(tapePath.begin(), tapePath.end()));
  EXPECT_TRUE(source->stereo());
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machine.tape()->load(std::move(source), "tape");
    machine.tape()->play();
  });
  ASSERT_TRUE(runLong(bench, playFrames));
  bench.dump(std::string(dumpName) + "-lesson");
  EXPECT_GT(differingPixels(logo, bench.runtime->captureFrame()), 10000U) << "磁带的数据线驱动了程序，画面应该从标志换成课文";
  EXPECT_TRUE(bench.errors->entries.empty());
}

TEST(PyramidTape, Pec9588RealSoundAndPictureTapeDrivesTheLessonScreenOfMenuItemEight) { lessonTapeDrivesTheLessonScreen("pec9588", "pec9588.bin", "Enter", "Digit8", "pec9588-8", 3600); }

TEST(PyramidTape, Pec586RealSoundAndPictureTapeDrivesTheLessonScreenOfMenuItemD) { lessonTapeDrivesTheLessonScreen("pec586", "pec586.bin", "Space", "KeyD", "pec586-d", 5400); }

TEST(PyramidTape, TheLeftChannelOfAStereoTapeReachesTheSpeaker) {
  auto deck = std::make_shared<ember::logic::hardware::TapeDeck>();
  const auto rom = readMachineRom("pec9588.bin");
  if (rom.empty()) GTEST_SKIP() << "没有找到 ember/roms/pec9588.bin";
  ember::bootstrap::PyramidMachine machine(ember::bootstrap::PyramidBoard::Model::Pec9588, rom, deck);
  machine.powerOn();
  deck->load(std::make_unique<LeftToneSource>(16000, 96000), "tone");
  deck->play();
  machine.runFrame();
  machine.runFrame();
  std::vector<float> samples;
  machine.takeAudio(samples);
  ASSERT_FALSE(samples.empty());
  const float peak = *std::max_element(samples.begin(), samples.end(), [](float a, float b) { return std::fabs(a) < std::fabs(b); });
  EXPECT_GT(std::fabs(peak), 0.1F) << "左声道的声音应该混进机器的声音输出";
}

}  // namespace
