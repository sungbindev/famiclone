/*
 * ------------------------------------------------------------------------
 * 名称：apu-rom-test.cpp
 * 说明：用公开的测试 ROM 验证经典 FC 的 APU：长度计数器、帧计数器与 IRQ 的时序、复位行为、DMC 基本操作与速率，以及与 IRQ、DMA 的相互作用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：ROM 来自 christopherpow/nes-test-roms，来源与用途见 core/test/data 各子目录的 README.md；
 *       结果读法与复位处理见 rom-test-support.hpp。暂时无法通过的项登记在 docs/02-硬件模块.md 第 12 节“已知差异”，
 *       对应测试（KnownDifferences）断言当前的失败结果，修复后需要同步改测试与文档。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "rom-test-support.hpp"

namespace {

using ember::test::FcRig;
using ember::test::ResultStyle;
using ember::test::RomCase;
using ember::test::RomResult;
using ember::test::runRom;

const RomCase ROMS[] = {
    // blargg_apu_2005.07.30：帧计数器与长度计数器的时序（结果显示在屏幕上，$01 为通过）
    {"blargg_01_len_ctr", "blargg-apu-tests/01.len_ctr.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_02_len_table", "blargg-apu-tests/02.len_table.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_03_irq_flag", "blargg-apu-tests/03.irq_flag.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_04_clock_jitter", "blargg-apu-tests/04.clock_jitter.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_05_len_timing_mode0", "blargg-apu-tests/05.len_timing_mode0.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_06_len_timing_mode1", "blargg-apu-tests/06.len_timing_mode1.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_07_irq_flag_timing", "blargg-apu-tests/07.irq_flag_timing.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_08_irq_timing", "blargg-apu-tests/08.irq_timing.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_09_reset_timing", "blargg-apu-tests/09.reset_timing.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_10_len_halt_timing", "blargg-apu-tests/10.len_halt_timing.nes", ResultStyle::Screen, 600, nullptr},
    {"blargg_11_len_reload_timing", "blargg-apu-tests/11.len_reload_timing.nes", ResultStyle::Screen, 600, nullptr},
    // apu_test（2010）：$6000 约定
    {"apu_test_1_len_ctr", "apu-test/1-len_ctr.nes", ResultStyle::Status, 600, nullptr},
    {"apu_test_2_len_table", "apu-test/2-len_table.nes", ResultStyle::Status, 600, nullptr},
    {"apu_test_3_irq_flag", "apu-test/3-irq_flag.nes", ResultStyle::Status, 600, nullptr},
    {"apu_test_4_jitter", "apu-test/4-jitter.nes", ResultStyle::Status, 600, nullptr},
    {"apu_test_5_len_timing", "apu-test/5-len_timing.nes", ResultStyle::Status, 600, nullptr},
    {"apu_test_6_irq_flag_timing", "apu-test/6-irq_flag_timing.nes", ResultStyle::Status, 600, nullptr},
    {"apu_test_7_dmc_basics", "apu-test/7-dmc_basics.nes", ResultStyle::Status, 1200, nullptr},
    {"apu_test_8_dmc_rates", "apu-test/8-dmc_rates.nes", ResultStyle::Status, 1200, nullptr},
    // apu_reset：上电与复位后的状态（测试要求复位时由辅助代码软复位）
    {"apu_reset_4015_cleared", "apu-reset/4015_cleared.nes", ResultStyle::Status, 1200, nullptr},
    {"apu_reset_4017_timing", "apu-reset/4017_timing.nes", ResultStyle::Status, 1200, nullptr},
    {"apu_reset_4017_written", "apu-reset/4017_written.nes", ResultStyle::Status, 1200, nullptr},
    {"apu_reset_irq_flag_cleared", "apu-reset/irq_flag_cleared.nes", ResultStyle::Status, 1200, nullptr},
    {"apu_reset_len_ctrs_enabled", "apu-reset/len_ctrs_enabled.nes", ResultStyle::Status, 1200, nullptr},
    {"apu_reset_works_immediately", "apu-reset/works_immediately.nes", ResultStyle::Status, 1200, nullptr},
    // cpu_interrupts_v2：IRQ 与 NMI、BRK、DMA 的相互作用（用到 APU 的帧 IRQ）
    {"cpu_interrupts_1_cli_latency", "cpu-interrupts/1-cli_latency.nes", ResultStyle::Status, 1200, nullptr},
    {"cpu_interrupts_4_irq_and_dma", "cpu-interrupts/4-irq_and_dma.nes", ResultStyle::Status, 1200, nullptr},
    {"cpu_interrupts_5_branch_delays_irq", "cpu-interrupts/5-branch_delays_irq.nes", ResultStyle::Status, 1200, nullptr},
    // DMC DMA 与 $2007 的读写（屏幕显示 Passed）
    {"dmc_dma_2007_write", "dmc-dma-during-read4/dma_2007_write.nes", ResultStyle::Screen, 1200, nullptr},
    {"dmc_dma_read_write_2007", "dmc-dma-during-read4/read_write_2007.nes", ResultStyle::Screen, 1200, nullptr},
};

class ApuRomTest : public testing::TestWithParam<RomCase> {};

TEST_P(ApuRomTest, ReportsAPass) {
  const RomCase& rom = GetParam();
  const RomResult result = runRom(rom);
  ASSERT_TRUE(result.finished) << "测试在 " << rom.maxFrames << " 帧内没有给出结果。屏幕：\n" << result.text;
  EXPECT_TRUE(result.passed) << result.text;
  if (rom.expectedText != nullptr) EXPECT_NE(result.text.find(rom.expectedText), std::string::npos) << "缺少预期输出：" << rom.expectedText << "\n实际：\n" << result.text;
}

INSTANTIATE_TEST_SUITE_P(Roms, ApuRomTest, testing::ValuesIn(ROMS), [](const testing::TestParamInfo<RomCase>& info) { return std::string(info.param.name); });

// ---------------------------------------------------------------- 已知差异

/** 屏幕上是否有一行恰好是 8 位十六进制数：旧框架的测试在输出与预期的校验和不一致时打印它。 */
bool hasChecksumLine(const std::string& text) {
  std::size_t begin = 0;
  while (begin < text.size()) {
    std::size_t end = text.find('\n', begin);
    if (end == std::string::npos) end = text.size();
    std::string line = text.substr(begin, end - begin);
    line.erase(0, line.find_first_not_of(' '));
    if (line.size() == 8 && std::all_of(line.begin(), line.end(), [](char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; })) return true;
    begin = end + 1;
  }
  return false;
}

std::string screenAfter(const char* path, int frames) {
  FcRig rig(path);
  rig.runFrames(frames);
  return rig.screenText();
}

TEST(ApuRomKnownDifferences, DmcDmaDoesNotRepeatTheCpuRead) {
  // DMC 取样 DMA 暂停 CPU 时，真机会在暂停的前几个周期里重复 CPU 正要读的地址（$2007、$4016 因此多读几次），
  // 这里的 CPU 暂停时不访问总线，所以三个依赖重复读的测试得不到预期的输出（校验和不一致或 Failed）。
  for (const char* path : {"dmc-dma-during-read4/dma_2007_read.nes", "dmc-dma-during-read4/double_2007_read.nes"}) {
    const std::string text = screenAfter(path, 900);
    EXPECT_TRUE(hasChecksumLine(text)) << path << "\n" << text;
    EXPECT_EQ(text.find("Passed"), std::string::npos) << path << "\n" << text;
  }
  const std::string controller = screenAfter("dmc-dma-during-read4/dma_4016_read.nes", 900);
  EXPECT_NE(controller.find("Failed"), std::string::npos) << controller;
}

TEST(ApuRomKnownDifferences, DmcDmaDoesNotDelaySpriteDma) {
  // 精灵 DMA 进行中发生 DMC DMA 时，真机的精灵 DMA 会被延后 2 个周期；这里两个 DMA 互不知道对方，所以总时间不对。
  for (const char* path : {"sprdma-and-dmc-dma/sprdma_and_dmc_dma.nes", "sprdma-and-dmc-dma/sprdma_and_dmc_dma_512.nes"}) {
    const RomResult result = runRom({"sprdma_and_dmc_dma", path, ResultStyle::Status, 1200, nullptr});
    ASSERT_TRUE(result.finished) << path << "\n" << result.text;
    EXPECT_FALSE(result.passed) << path;
    EXPECT_NE(result.text.find("Failed"), std::string::npos) << path << "\n" << result.text;
  }
}

TEST(ApuRomKnownDifferences, NmiHijackingOfBrkAndIrq) {
  // cpu_interrupts_v2 的第 2、3 项检查 NMI 劫持 BRK、IRQ 的向量时序，CPU 核在这一点上与真机不同；与 APU 无关，在 CPU 的已知差异里登记。
  for (const char* path : {"cpu-interrupts/2-nmi_and_brk.nes", "cpu-interrupts/3-nmi_and_irq.nes"}) {
    const RomResult result = runRom({"cpu_interrupts", path, ResultStyle::Status, 1200, nullptr});
    ASSERT_TRUE(result.finished) << path << "\n" << result.text;
    EXPECT_FALSE(result.passed) << path;
  }
}

}  // namespace
