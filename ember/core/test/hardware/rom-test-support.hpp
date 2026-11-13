/*
 * ------------------------------------------------------------------------
 * 名称：rom-test-support.hpp
 * 说明：用公开测试 ROM 验证经典 FC 的公共辅助：ROM 清单项、运行到出结果、两种结果读法（屏幕、$6000 约定）、测试要求复位时的软复位。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：结果有两种读法：2005 年的 blargg 测试把结果显示在屏幕上（把名称表 0 的图块号当 ASCII 读出），
 *       2010 年的测试按 $6000 的约定写结果（$6000 为状态，$6001-$6003 为签名 DE B0 61，$6004 起为文本；
 *       $80 表示还在运行，$81 表示测试要求至少过 100 毫秒后按复位键，其余 $00-$7F 是最终结果，0 为通过）。
 *       卡带用真实的 nrom 插件装配（需要别的 mapper 时由 ROM 清单项指定测试插件），没有 mapper 的替身实现。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "fc-test-support.hpp"

namespace ember::test {

/** 结果的读法。 */
enum class ResultStyle {
  /** 屏幕上显示 PASSED、Passed、FAILED #n、Failed，或 blargg 2005 测试的 $01（通过）。 */
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
  /** 除 nrom 以外还要安装的测试插件（core/test/data/test-plugins 下的目录名），没有则为空。 */
  const char* testPlugin = nullptr;
};

struct RomResult {
  bool finished{false};
  bool passed{false};
  std::string text;
};

/** 文字里是否有 “$” 加两位十六进制数（blargg 2005 年的测试用它显示结果码），有则返回这个码，否则返回 -1；文字还没打完时返回 -1。 */
inline int findResultCode(const std::string& text) {
  const std::size_t dollar = text.find('$');
  if (dollar == std::string::npos || dollar + 2 >= text.size()) return -1;
  if (!std::isxdigit(static_cast<unsigned char>(text[dollar + 1])) || !std::isxdigit(static_cast<unsigned char>(text[dollar + 2]))) return -1;
  return std::stoi(text.substr(dollar + 1, 2), nullptr, 16);
}

/** 测试要求复位（状态 $81）后，等待多少帧再按复位键（至少 100 毫秒，约 6 帧）。 */
inline constexpr int RESET_WAIT_FRAMES = 8;
/** 复位之后多少帧内不再检查是否要求复位（等测试自己把状态改回 $80）。 */
inline constexpr int RESET_SETTLE_FRAMES = 4;

/** 运行 ROM 直到给出结果；状态读法下遇到“要求复位”就软复位后继续。 */
inline RomResult runRom(const RomCase& rom) {
  std::vector<std::vector<std::uint8_t>> plugins;
  if (rom.testPlugin != nullptr) plugins.push_back(testPluginPackage(rom.testPlugin));
  FcRig rig(rom.path, plugins);
  RomResult result;
  int resetRequestedAt = -1;
  int lastResetAt = -1000;
  for (int frame = 0; frame < rom.maxFrames; ++frame) {
    rig.machine.runFrame();
    if (rom.style == ResultStyle::Screen) {
      if (frame % 4 != 3) continue;
      const std::string text = rig.screenText();
      const int code = findResultCode(text);
      const bool passed = text.find("PASSED") != std::string::npos || text.find("Passed") != std::string::npos || code == 1;
      const bool failed = text.find("FAILED #") != std::string::npos || text.find("Failed") != std::string::npos || code > 1;
      if (passed || failed) {
        result.finished = true;
        result.passed = passed;
        result.text = text;
        return result;
      }
    } else {
      const auto memory = rig.prgRam();
      const bool signature = memory[1] == 0xDE && memory[2] == 0xB0 && memory[3] == 0x61;
      if (!signature) continue;
      if (memory[0] == 0x81) {
        if (frame - lastResetAt < RESET_SETTLE_FRAMES) continue;
        if (resetRequestedAt < 0) resetRequestedAt = frame;
        if (frame - resetRequestedAt >= RESET_WAIT_FRAMES) {
          rig.machine.softReset();
          resetRequestedAt = -1;
          lastResetAt = frame;
        }
        continue;
      }
      resetRequestedAt = -1;
      // $80 表示还在运行。
      if (memory[0] != 0x80) {
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

}  // namespace ember::test
