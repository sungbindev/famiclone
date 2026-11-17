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
TEST(Sb97Diag, BootIo) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / u8"小霸王" / "SB97.nes";
  std::ifstream stream(path, std::ios::binary);
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  ember::bootstrap::Sb97Machine machine(std::vector<std::uint8_t>(file.begin() + 16, file.end()));
  machine.powerOn();
  int frame = 0;
  for (const int end : {60, 300, 700, 1500, 3000, 3300}) {
    for (; frame < end; ++frame) machine.runFrame();
    hw::RecorderSpec spec;
    spec.kind = hw::RecorderKind::Io;
    spec.maxLines = 60000;
    const auto id = machine.debugger()->startRecorder(spec);
    for (int i = 0; i < 20; ++i, ++frame) machine.runFrame();
    const auto out = machine.debugger()->stopRecorder(id);
    std::map<std::string, int> counts;
    for (const auto& line : out.lines) {
      std::istringstream in(line);
      std::string rw, addr;
      in >> rw >> addr;
      if (addr == "$2000" || addr == "$2001" || addr == "$2002" || addr == "$2003" || addr == "$2004" || addr == "$2005" || addr == "$2006" || addr == "$2007") continue;
      ++counts[rw + " " + addr];
    }
    std::printf("== frames up to %d (lines %zu%s):", end, out.lines.size(), out.truncated ? " truncated" : "");
    std::vector<std::pair<int, std::string>> sorted;
    for (const auto& [key, n] : counts) sorted.emplace_back(n, key);
    std::sort(sorted.rbegin(), sorted.rend());
    int shown = 0;
    for (const auto& [n, key] : sorted) { if (shown++ >= 14) break; std::printf(" [%s x%d]", key.c_str(), n); }
    std::printf("\n");
  }
}
}  // namespace
