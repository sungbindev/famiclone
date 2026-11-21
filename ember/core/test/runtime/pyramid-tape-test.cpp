/*
 * ------------------------------------------------------------------------
 * 名称：pyramid-tape-test.cpp
 * 说明：验证金字塔学习机的磁带：真实的声像教学磁带驱动菜单 8 的课文画面；立体声磁带的左声道经主机送到扬声器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：真实 ROM 在仓库的 ember/roms，真实磁带在 reference/tapes（m4a，由 FFmpeg 库解码），缺它们时用例跳过；
 *       放真实磁带要约 60 到 90 秒的磁带时间（模拟时间，不是真实时间），所以分段运行，避开单次等待 10 秒的限制。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
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

/** 分段运行的帧数：每段不超过测试支持里单次等待的 10 秒。 */
constexpr std::uint64_t CHUNK_FRAMES = 450;

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

/** 开机，离开开机画面（firstKey），从主菜单进入指定的项（按键名如 "Digit5"）。 */
void enterMenuItem(RealRomBench& bench, const char* machine, const std::vector<std::uint8_t>& rom, const char* firstKey, const char* key, std::uint64_t settle) {
  bench.bootMachine(machine, rom);
  ASSERT_TRUE(bench.run(300));
  bench.tap(firstKey, 8, 420);
  bench.tap(key, 8, settle);
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
