/*
 * ------------------------------------------------------------------------
 * 名称：ppu-rom-test.cpp
 * 说明：用公开的测试 ROM 验证经典 FC 的 PPU（含 CPU 与 PPU 的相位、vblank 与 NMI 的时序、精灵命中与溢出、OAM 与读缓冲）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：ROM 来自 christopherpow/nes-test-roms，来源与用途见 core/test/data 各子目录的 README.md；
 *       结果有两种读法：2005 年的 blargg 测试把结果显示在屏幕上（把名称表 0 的图块号当 ASCII 读出），
 *       2010 年的测试按 $6000 的约定写结果（$6000 为状态，$6001-$6003 为签名 DE B0 61，$6004 起为文本）。
 *       暂时无法通过的项见 docs/02-硬件模块.md 第 6.1 节“已知差异”，对应测试断言当前的失败结果，修复后需要同步改测试与文档。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cctype>
#include <cstring>
#include <string>

#include "fc-test-support.hpp"

namespace {

using ember::test::FcRig;

/** 结果的读法。 */
enum class ResultStyle {
  /** 屏幕上显示 PASSED、FAILED #n，或 blargg 2005 PPU 测试的 $01（通过）。 */
  Screen,
  /** $6000 约定：状态 0 为通过。 */
  Status
};

struct RomCase {
  const char* name;
  const char* path;
  ResultStyle style;
  /** 最多运行的帧数，超过仍没有结果算失败。 */
  int maxFrames;
  /** 文本里必须出现的内容（核对表格输出），为空则不核对。 */
  const char* expectedText;
};

struct RomResult {
  bool finished{false};
  bool passed{false};
  std::string text;
};

/** 文字里是否有 “$” 加两位十六进制数（blargg 2005 年的 PPU 测试用它显示结果码），有则返回这个码，否则返回 -1；文字还没打完时返回 -1。 */
int findResultCode(const std::string& text) {
  const std::size_t dollar = text.find('$');
  if (dollar == std::string::npos || dollar + 2 >= text.size()) return -1;
  if (!std::isxdigit(static_cast<unsigned char>(text[dollar + 1])) || !std::isxdigit(static_cast<unsigned char>(text[dollar + 2]))) return -1;
  return std::stoi(text.substr(dollar + 1, 2), nullptr, 16);
}

RomResult runRom(const RomCase& rom) {
  FcRig rig(rom.path);
  RomResult result;
  for (int frame = 0; frame < rom.maxFrames; ++frame) {
    rig.machine.runFrame();
    if (frame % 4 != 3) continue;
    if (rom.style == ResultStyle::Screen) {
      const std::string text = rig.screenText();
      const int code = findResultCode(text);
      const bool passed = text.find("PASSED") != std::string::npos || code == 1;
      const bool failed = text.find("FAILED #") != std::string::npos || code > 1;
      if (passed || failed) {
        result.finished = true;
        result.passed = passed;
        result.text = text;
        return result;
      }
    } else {
      const auto memory = rig.cartridge.prgRam();
      const bool signature = memory[1] == 0xDE && memory[2] == 0xB0 && memory[3] == 0x61;
      // $80 表示还在运行，$81 表示测试要求复位。
      if (signature && memory[0] != 0x80 && memory[0] != 0x81) {
        result.finished = true;
        result.passed = memory[0] == 0;
        result.text = std::string(reinterpret_cast<const char*>(memory.data() + 4), strnlen(reinterpret_cast<const char*>(memory.data() + 4), memory.size() - 4));
        return result;
      }
    }
  }
  result.text = rig.screenText();
  return result;
}

const RomCase ROMS[] = {
    // blargg_ppu_tests_2005.09.15b
    {"blargg_palette_ram", "blargg-ppu-tests/palette_ram.nes", ResultStyle::Screen, 400, nullptr},
    {"blargg_power_up_palette", "blargg-ppu-tests/power_up_palette.nes", ResultStyle::Screen, 400, nullptr},
    {"blargg_sprite_ram", "blargg-ppu-tests/sprite_ram.nes", ResultStyle::Screen, 400, nullptr},
    {"blargg_vbl_clear_time", "blargg-ppu-tests/vbl_clear_time.nes", ResultStyle::Screen, 400, nullptr},
    {"blargg_vram_access", "blargg-ppu-tests/vram_access.nes", ResultStyle::Screen, 400, nullptr},
    // sprite_hit_tests_2005.10.05
    {"sprite_hit_01_basics", "sprite-hit-tests/01.basics.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_02_alignment", "sprite-hit-tests/02.alignment.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_03_corners", "sprite-hit-tests/03.corners.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_04_flip", "sprite-hit-tests/04.flip.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_05_left_clip", "sprite-hit-tests/05.left_clip.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_06_right_edge", "sprite-hit-tests/06.right_edge.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_07_screen_bottom", "sprite-hit-tests/07.screen_bottom.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_08_double_height", "sprite-hit-tests/08.double_height.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_09_timing_basics", "sprite-hit-tests/09.timing_basics.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_10_timing_order", "sprite-hit-tests/10.timing_order.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_hit_11_edge_timing", "sprite-hit-tests/11.edge_timing.nes", ResultStyle::Screen, 400, nullptr},
    // sprite_overflow_tests
    {"sprite_overflow_1_basics", "sprite-overflow-tests/1.Basics.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_overflow_2_details", "sprite-overflow-tests/2.Details.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_overflow_3_timing", "sprite-overflow-tests/3.Timing.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_overflow_4_obscure", "sprite-overflow-tests/4.Obscure.nes", ResultStyle::Screen, 400, nullptr},
    {"sprite_overflow_5_emulator", "sprite-overflow-tests/5.Emulator.nes", ResultStyle::Screen, 400, nullptr},
    // vbl_nmi_timing（2005）
    {"vbl_nmi_timing_1_frame_basics", "vbl-nmi-timing/1.frame_basics.nes", ResultStyle::Screen, 400, nullptr},
    {"vbl_nmi_timing_2_vbl_timing", "vbl-nmi-timing/2.vbl_timing.nes", ResultStyle::Screen, 400, nullptr},
    {"vbl_nmi_timing_3_even_odd_frames", "vbl-nmi-timing/3.even_odd_frames.nes", ResultStyle::Screen, 400, nullptr},
    {"vbl_nmi_timing_4_vbl_clear_timing", "vbl-nmi-timing/4.vbl_clear_timing.nes", ResultStyle::Screen, 400, nullptr},
    {"vbl_nmi_timing_5_nmi_suppression", "vbl-nmi-timing/5.nmi_suppression.nes", ResultStyle::Screen, 400, nullptr},
    {"vbl_nmi_timing_6_nmi_disable", "vbl-nmi-timing/6.nmi_disable.nes", ResultStyle::Screen, 400, nullptr},
    {"vbl_nmi_timing_7_nmi_timing", "vbl-nmi-timing/7.nmi_timing.nes", ResultStyle::Screen, 400, nullptr},
    // ppu_vbl_nmi（2010）：表格输出与测试说明里的正确输出一致
    {"ppu_vbl_nmi_01_vbl_basics", "ppu-vbl-nmi/01-vbl_basics.nes", ResultStyle::Status, 400, nullptr},
    {"ppu_vbl_nmi_02_vbl_set_time", "ppu-vbl-nmi/02-vbl_set_time.nes", ResultStyle::Status, 400, "03 - V\n04 - -\n05 V -"},
    {"ppu_vbl_nmi_03_vbl_clear_time", "ppu-vbl-nmi/03-vbl_clear_time.nes", ResultStyle::Status, 400, "05 V\n06 -"},
    {"ppu_vbl_nmi_04_nmi_control", "ppu-vbl-nmi/04-nmi_control.nes", ResultStyle::Status, 400, nullptr},
    {"ppu_vbl_nmi_05_nmi_timing", "ppu-vbl-nmi/05-nmi_timing.nes", ResultStyle::Status, 400, "02 4\n03 3"},
    {"ppu_vbl_nmi_06_suppression", "ppu-vbl-nmi/06-suppression.nes", ResultStyle::Status, 400, "03 - N\n04 - -\n05 V -"},
    {"ppu_vbl_nmi_07_nmi_on_timing", "ppu-vbl-nmi/07-nmi_on_timing.nes", ResultStyle::Status, 400, "04 N\n05 -"},
    {"ppu_vbl_nmi_08_nmi_off_timing", "ppu-vbl-nmi/08-nmi_off_timing.nes", ResultStyle::Status, 400, "06 -\n07 N"},
    {"ppu_vbl_nmi_09_even_odd_frames", "ppu-vbl-nmi/09-even_odd_frames.nes", ResultStyle::Status, 400, "00 01 01 02"},
    {"ppu_vbl_nmi_10_even_odd_timing", "ppu-vbl-nmi/10-even_odd_timing.nes", ResultStyle::Status, 400, "08 08 09 07"},
    // OAM 与读缓冲
    {"oam_read", "oam-read/oam_read.nes", ResultStyle::Status, 1000, nullptr},
    {"oam_stress", "oam-stress/oam_stress.nes", ResultStyle::Status, 3000, nullptr},
    {"ppu_read_buffer", "ppu-read-buffer/test_ppu_read_buffer.nes", ResultStyle::Status, 3000, nullptr},
};

class PpuRomTest : public testing::TestWithParam<RomCase> {};

TEST_P(PpuRomTest, ReportsAPass) {
  const RomCase& rom = GetParam();
  const RomResult result = runRom(rom);
  ASSERT_TRUE(result.finished) << "测试在 " << rom.maxFrames << " 帧内没有给出结果。屏幕：\n" << result.text;
  EXPECT_TRUE(result.passed) << result.text;
  if (rom.expectedText != nullptr) EXPECT_NE(result.text.find(rom.expectedText), std::string::npos) << "缺少预期输出：" << rom.expectedText << "\n实际：\n" << result.text;
}

INSTANTIATE_TEST_SUITE_P(Roms, PpuRomTest, testing::ValuesIn(ROMS), [](const testing::TestParamInfo<RomCase>& info) { return std::string(info.param.name); });

// ---------------------------------------------------------------- 已知差异

TEST(PpuRomKnownDifferences, OpenBusDecayIsNotSimulated) {
  // ppu_open_bus 的第 3 项要求 PPU 数据总线的值在约 1 秒后衰减为 0；本阶段把开放总线简化为“保持最近一次值”，所以失败在第 3 项。
  const RomResult result = runRom({"ppu_open_bus", "ppu-open-bus/ppu_open_bus.nes", ResultStyle::Status, 1000, nullptr});
  ASSERT_TRUE(result.finished) << result.text;
  EXPECT_FALSE(result.passed);
  EXPECT_NE(result.text.find("Failed #3"), std::string::npos) << result.text;
  EXPECT_NE(result.text.find("Decay value should become zero"), std::string::npos) << result.text;
}

}  // namespace
