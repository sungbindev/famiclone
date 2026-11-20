/*
 * ------------------------------------------------------------------------
 * 名称：qls-test.cpp
 * 说明：用真实的声像教学卡 qls.nes（mapper 172）与真实的声像磁带验证 qls 插件：卡带启动，磁带机放出磁带后右声道的下降沿经插件变成中断与计时，画面随磁带推进。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：ROM 在 reference/study_roms，磁带在 reference/tapes（m4a，由 FFmpeg 库解码）；缺任何一个时用例跳过；设置环境变量 EMBER_REAL_ROM_DUMP 为目录时，把关键时刻的画面存成 PPM 供人工查看。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "../support/real-rom-support.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "infra/media/disk-tape-store.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::test::firstTape;
using ember::test::readReferenceFile;
using ember::test::RealRomBench;

TEST(QlsCard, BootsAndTheTapeDrivesTheScreen) {
  const auto rom = readReferenceFile("study_roms/普里奇/qls.nes");
  const auto tape = firstTape();
  if (rom.empty() || tape.empty()) GTEST_SKIP() << "没有找到 qls.nes 或声像磁带";
  RealRomBench bench({"qls"});
  bench.boot(rom);
  ASSERT_TRUE(bench.run(300));
  bench.dump("qls-0-boot");
  EXPECT_TRUE(bench.errors->entries.empty());

  ember::infra::media::DiskTapeStore store(nullptr);
  const auto tapePath = tape.u8string();
  auto source = store.open(std::string(tapePath.begin(), tapePath.end()));
  EXPECT_TRUE(source->stereo());
  bench.runtime->useMachine([&](hw::Machine& machine) {
    machine.tape()->load(std::move(source), "tape");
    machine.tape()->play();
  });
  const auto title = bench.runtime->captureFrame();
  for (int second = 1; second <= 8; ++second) {
    ASSERT_TRUE(bench.run(300));
    bench.dump("qls-play-" + std::to_string(second * 5));
  }
  EXPECT_FALSE(ember::test::sameFrame(title, bench.runtime->captureFrame())) << "磁带放出的数据线驱动了卡带，画面应该从标题换成课文";
  EXPECT_TRUE(bench.errors->entries.empty());
}

}  // namespace
