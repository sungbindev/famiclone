// 临时诊断：不提交。按 SB97_SCRIPT 里的步骤驱动机器：frames:N、tap:Code、wait:N、shot:名字、disk:路径（盘片镜像）。
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>

#include "bootstrap/machines/sb97/sb97.hpp"

namespace {
namespace hw = ember::contracts::hardware;
using ember::bootstrap::Sb97Machine;

std::vector<std::uint8_t> readFile(const std::filesystem::path& path, std::size_t skip) {
  std::ifstream stream(path, std::ios::binary);
  std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  return std::vector<std::uint8_t>(file.begin() + static_cast<std::ptrdiff_t>(skip), file.end());
}

std::string env(const char* name) {
  char* value = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return {};
  std::string result(value);
  std::free(value);
  return result;
}

void shot(Sb97Machine& machine, const std::string& name) {
  const hw::FrameBuffer picture = machine.frame();
  std::ofstream out(env("TEMP") + "\\sb97s." + name + ".ppm", std::ios::binary);
  out << "P6\n256 240\n255\n";
  for (const std::uint32_t pixel : picture.pixels) {
    const char rgb[3] = {static_cast<char>(pixel & 0xFF), static_cast<char>((pixel >> 8) & 0xFF), static_cast<char>((pixel >> 16) & 0xFF)};
    out.write(rgb, 3);
  }
}

TEST(Sb97Diag, Script) {
  const std::filesystem::path path = std::filesystem::path(EMBER_TEST_DATA_DIR) / ".." / ".." / ".." / ".." / "reference" / "rom" / u8"小霸王" / "SB97.nes";
  Sb97Machine machine(readFile(path, 16));
  const std::string script = env("SB97_SCRIPT");
  std::stringstream steps(script);
  std::string step;
  bool powered = false;
  while (std::getline(steps, step, ';')) {
    const auto colon = step.find(':');
    const std::string verb = step.substr(0, colon);
    const std::string arg = colon == std::string::npos ? "" : step.substr(colon + 1);
    if (verb == "disk") {
      machine.floppyDrive(0)->insertImage(readFile(std::filesystem::u8path(arg), 0), false, "disk.img");
    } else if (verb == "frames" || verb == "wait") {
      if (!powered) { machine.powerOn(); powered = true; }
      for (int i = 0, n = std::atoi(arg.c_str()); i < n; ++i) machine.runFrame();
    } else if (verb == "tap") {
      machine.input(hw::KeyInput{arg, true});
      for (int i = 0; i < 8; ++i) machine.runFrame();
      machine.input(hw::KeyInput{arg, false});
      for (int i = 0; i < 8; ++i) machine.runFrame();
    } else if (verb == "shot") {
      shot(machine, arg);
    }
  }
}
}  // namespace
