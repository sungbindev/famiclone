// 临时诊断：不提交。
#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "bootstrap/machines/sb97/sb97.hpp"

namespace {
namespace hw = ember::contracts::hardware;
TEST(Sb97Diag, SplitLines) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / u8"小霸王" / "SB97.nes";
  std::ifstream stream(path, std::ios::binary);
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ember::bootstrap::Sb97Machine machine(std::vector<std::uint8_t>(file.begin() + 16, file.end()));
  machine.powerOn();
  for (int frame = 0; frame < 4200; ++frame) machine.runFrame();
  int printed = 0;
  for (int i = 0; i < 4000000 && printed < 40; ++i) {
    machine.debugger()->step(hw::StepUnit::Instruction, 1);
    const auto pc = machine.cpuRegisters().pc;
    // 执行前的 PC 已经变了，这里用“刚执行完”的 PC 判断：8F69 的下一条 8F6C 等
    if (pc == 0x8F6C || pc == 0x8F9F || pc == 0x8FC8 || pc == 0x8FCB || pc == 0x8FD8 || pc == 0x8F97 || pc == 0x8F61 || pc == 0x8FD9 || pc == 0x8FE8 || pc == 0x8FF0 || pc == 0x9004 || pc == 0x9009 || pc == 0x900F || pc == 0x9014 || pc == 0x9017) {
      std::printf("pc=%04X line=%d dot=%d\n", pc, machine.ppu().scanline(), machine.ppu().dot());
      ++printed;
    }
  }
}
}  // namespace
