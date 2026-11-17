// 临时诊断：不提交。打印各名称表每一行是否有非零图块。
#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "bootstrap/machines/sb97/sb97.hpp"

namespace {
TEST(Sb97Diag, Nametables) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / u8"小霸王" / "SB97.nes";
  std::ifstream stream(path, std::ios::binary);
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ember::bootstrap::Sb97Machine machine(std::vector<std::uint8_t>(file.begin() + 16, file.end()));
  machine.powerOn();
  for (int frame = 0; frame < 4200; ++frame) machine.runFrame();
  auto& nt = machine.board().nametable();
  for (int table = 0; table < 4; ++table) {
    std::printf("NT%d rows nonzero: ", table);
    for (int row = 0; row < 30; ++row) {
      int count = 0;
      for (int col = 0; col < 32; ++col) if (nt[table * 1024 + row * 32 + col] != 0) ++count;
      std::printf("%d ", count);
    }
    std::printf("\n");
  }
  std::printf("2000 ctl=%02X 2001=%02X scrollX/Y shadows %02X %02X\n", machine.peekCpuBus(0x102), machine.peekCpuBus(0x103), machine.peekCpuBus(0x104), machine.peekCpuBus(0x105));
  std::printf("$38=%02X $39=%02X $0E=%02X $0F=%02X $10=%02X\n", machine.peekCpuBus(0x38), machine.peekCpuBus(0x39), machine.peekCpuBus(0x0E), machine.peekCpuBus(0x0F), machine.peekCpuBus(0x10));
}
}  // namespace
