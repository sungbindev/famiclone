// 临时诊断：不提交。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>

#include "bootstrap/machines/sb97/sb97.hpp"

namespace {
namespace hw = ember::contracts::hardware;
TEST(Sb97Diag, EarlyBoot) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / u8"小霸王" / "SB97.nes";
  std::ifstream stream(path, std::ios::binary);
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ember::bootstrap::Sb97Machine machine(std::vector<std::uint8_t>(file.begin() + 16, file.end()));
  machine.powerOn();
  for (int f = 0; f < 1000; ++f) machine.runFrame();
  std::vector<int> prev(0x400);
  for (int a = 0; a < 0x400; ++a) prev[a] = machine.peekCpuBus(static_cast<std::uint16_t>(a));
  std::map<int, std::vector<int>> changes;
  for (int f = 1000; f < 3200; ++f) {
    machine.runFrame();
    for (int a = 0; a < 0x400; ++a) {
      const int v = machine.peekCpuBus(static_cast<std::uint16_t>(a));
      if (v != prev[a]) { changes[a].push_back(f); prev[a] = v; }
    }
  }
  for (auto& [a, fr] : changes) {
    if (fr.size() > 12) continue;
    std::printf("addr %04X n=%zu frames:", a, fr.size());
    for (int x : fr) std::printf(" %d", x);
    std::printf("\n");
  }}
}  // namespace
