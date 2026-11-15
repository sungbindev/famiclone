/*
 * ------------------------------------------------------------------------
 * 名称：family-basic-test.cpp
 * 说明：用真实的 Family BASIC（v3.0）验证经典 FC 的 Family BASIC 键盘与数据录音机：启动、键盘输入出现在画面上、没接键盘时按键不进机器、SAVE 把程序录进磁带机再 LOAD 读回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：ROM 在 reference/nes-games，没有这个文件时用例跳过；设置环境变量 EMBER_REAL_ROM_DUMP 为目录时，把关键时刻的画面存成 PPM 供人工查看。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "../support/real-rom-support.hpp"
#include "contracts/hardware/tape-deck.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::test::countRepeats;
using ember::test::differingRows;
using ember::test::readReferenceFile;
using ember::test::RealRomBench;

const char* const FAMILY_BASIC_ROM = "nes-games/Family BASIC (Japan) (v3.0).nes";

TEST(FamilyBasic, BootsAndShowsWhatIsTypedOnTheKeyboard) {
  const auto rom = readReferenceFile(FAMILY_BASIC_ROM);
  if (rom.empty()) GTEST_SKIP() << "没有找到 Family BASIC 的 ROM";
  RealRomBench bench;
  bench.boot(rom, true);
  ASSERT_TRUE(bench.run(300));
  bench.dump("fbasic-1-boot");
  const auto booted = bench.runtime->captureFrame();
  for (const char* code : {"KeyP", "KeyR", "KeyI", "KeyN", "KeyT"}) bench.tap(code);
  ASSERT_TRUE(bench.run(10));
  bench.dump("fbasic-2-typed");
  EXPECT_FALSE(differingRows(booted, bench.runtime->captureFrame()).empty()) << "键入的字母应该出现在画面上";
}

TEST(FamilyBasic, TheKeyboardDoesNothingUntilItIsConnected) {
  const auto rom = readReferenceFile(FAMILY_BASIC_ROM);
  if (rom.empty()) GTEST_SKIP() << "没有找到 Family BASIC 的 ROM";
  RealRomBench bench;
  bench.boot(rom, false);
  ASSERT_TRUE(bench.run(300));
  const auto booted = bench.runtime->captureFrame();
  for (const char* code : {"KeyP", "KeyR", "KeyI", "KeyN", "KeyT"}) bench.tap(code);
  ASSERT_TRUE(bench.run(10));
  EXPECT_TRUE(differingRows(booted, bench.runtime->captureFrame()).empty()) << "没接键盘时按键不应该进入机器";
}

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

TEST(FamilyBasic, SaveRecordsTheProgramAndLoadReadsItBack) {
  const auto rom = readReferenceFile(FAMILY_BASIC_ROM);
  if (rom.empty()) GTEST_SKIP() << "没有找到 Family BASIC 的 ROM";
  RealRomBench bench;
  bench.boot(rom, true);
  ASSERT_TRUE(bench.run(300));
  const auto blank = bench.runtime->captureFrame();
  // 输入一行程序：10 PRINT 7
  for (const char* code : {"Digit1", "Digit0", "Space", "KeyP", "KeyR", "KeyI", "KeyN", "KeyT", "Space", "Digit7", "Enter"}) bench.tap(code);
  ASSERT_TRUE(bench.run(20));
  bench.dump("fbasic-4-program");
  const auto typed = bench.runtime->captureFrame();
  const auto lineRows = differingRows(blank, typed);
  ASSERT_FALSE(lineRows.empty()) << "键入的程序行应该出现在画面上";

  auto recorded = std::make_shared<std::vector<std::int16_t>>();
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->record(std::make_unique<MemorySink>(recorded), "save"); });
  // SAVE "A"
  for (const char* code : {"KeyS", "KeyA", "KeyV", "KeyE"}) bench.tap(code);
  bench.tapShifted("Digit2");
  bench.tap("KeyA");
  bench.tapShifted("Digit2");
  bench.tap("Enter");
  ASSERT_TRUE(bench.run(1320));
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->stop(); });
  EXPECT_GT(recorded->size(), 32000U) << "SAVE 应该录进了至少一秒的信号";

  // 清掉程序，再从磁带读回。
  for (const char* code : {"KeyN", "KeyE", "KeyW", "Enter"}) bench.tap(code);
  ASSERT_TRUE(bench.run(20));
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->load(std::make_unique<ReplaySource>(*recorded), "load"); });
  for (const char* code : {"KeyL", "KeyO", "KeyA", "KeyD"}) bench.tap(code);
  bench.tapShifted("Digit2");
  bench.tap("KeyA");
  bench.tapShifted("Digit2");
  bench.tap("Enter");
  ASSERT_TRUE(bench.run(60));
  bench.runtime->useMachine([&](hw::Machine& machine) { machine.tape()->play(); });
  ASSERT_TRUE(bench.run(1500));
  for (const char* code : {"KeyL", "KeyI", "KeyS", "KeyT", "Enter"}) bench.tap(code);
  ASSERT_TRUE(bench.run(30));
  bench.dump("fbasic-8-list");
  // LIST 的输出里应该有和当初键入的那一行完全一样的一行（除了当初那一行本身）。
  EXPECT_GE(countRepeats(typed, bench.runtime->captureFrame(), lineRows), 2U) << "LOAD 之后 LIST 应该列出读回的程序";
}

}  // namespace
