/*
 * ------------------------------------------------------------------------
 * 名称：nestest-test.cpp
 * 说明：用 nestest 验证 CPU 核：自动模式从 $C000 起运行，逐行比对日志里的 PC、A、X、Y、P、SP 与累计周期数（忽略 PPU 列），并核对 $0002、$0003 为 0。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：数据在 core/test/data/nestest/，来源见该目录的 README.md；日志共 8991 行，必须全部一致。
 *       ROM 的复位向量是 $C004（交互模式的入口），自动模式要从 $C000 起，所以由测试用的内存把复位向量读成 $C000，
 *       让 CPU 经完整的复位序列到达 $C000：寄存器与累计周期（7）正好等于日志第一行。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "logic/hardware/bus/mapped-bus.hpp"
#include "logic/hardware/cpu-6502/cpu-6502.hpp"
#include "logic/hardware/lines/wired-or-line.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

#ifndef EMBER_TEST_DATA_DIR
#error "EMBER_TEST_DATA_DIR 由 CMake 传入：测试数据目录"
#endif

namespace {

namespace hw = ember::contracts::hardware;

constexpr std::size_t NES_HEADER_SIZE = 16;
constexpr std::size_t PRG_SIZE = 16 * 1024;
constexpr std::size_t EXPECTED_LOG_LINES = 8991;

/** 读整个文件；读不到返回空。 */
std::vector<std::uint8_t> readBinary(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

/** 按行读文本文件，去掉行尾的回车。 */
std::vector<std::string> readLines(const std::string& path) {
  std::ifstream file(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

/** 2K 内存（镜像到 $1FFF）。 */
class Ram final : public hw::BusDevice {
 public:
  hw::BusRead read(std::uint16_t address) override { return hw::driveBus(bytes[address & 0x07FF]); }
  void write(std::uint16_t address, std::uint8_t value) override { bytes[address & 0x07FF] = value; }

  std::uint8_t bytes[0x0800]{};
};

/** 16K 的 PRG-ROM（镜像到 $8000-$FFFF）；复位向量读成 $C000。 */
class Rom final : public hw::BusDevice {
 public:
  explicit Rom(std::vector<std::uint8_t> prg) : prg_(std::move(prg)) {}

  hw::BusRead read(std::uint16_t address) override {
    if (address == 0xFFFC) return hw::driveBus(0x00);
    if (address == 0xFFFD) return hw::driveBus(0xC0);
    return hw::driveBus(prg_[address & (PRG_SIZE - 1)]);
  }

  void write(std::uint16_t /*address*/, std::uint8_t /*value*/) override {}

 private:
  std::vector<std::uint8_t> prg_;
};

/** 把寄存器格式化成与日志相同的文本。 */
std::string describe(std::uint16_t pc, std::uint8_t a, std::uint8_t x, std::uint8_t y, std::uint8_t p, std::uint8_t sp, std::uint64_t cycles) {
  char text[96];
  std::snprintf(text, sizeof(text), "%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%llu", pc, a, x, y, p, sp, static_cast<unsigned long long>(cycles));
  return text;
}

/** 从日志行取出 PC、A、X、Y、P、SP 与 CYC（忽略 PPU 列）。 */
std::string describeLogLine(const std::string& line) {
  static const std::regex pattern(R"(^([0-9A-F]{4}) .*A:([0-9A-F]{2}) X:([0-9A-F]{2}) Y:([0-9A-F]{2}) P:([0-9A-F]{2}) SP:([0-9A-F]{2}) PPU:.*CYC:(\d+)$)");
  std::smatch match;
  if (!std::regex_match(line, match, pattern)) return "无法解析的日志行：" + line;
  return match[1].str() + " A:" + match[2].str() + " X:" + match[3].str() + " Y:" + match[4].str() + " P:" + match[5].str() + " SP:" + match[6].str() + " CYC:" + match[7].str();
}

TEST(Nestest, AutomatedRunMatchesTheLogLineByLine) {
  const std::string directory = std::string(EMBER_TEST_DATA_DIR) + "/nestest/";
  const std::vector<std::uint8_t> rom = readBinary(directory + "nestest.nes");
  const std::vector<std::string> log = readLines(directory + "nestest.log");
  ASSERT_GE(rom.size(), NES_HEADER_SIZE + PRG_SIZE) << "找不到 nestest.nes，见 core/test/data/nestest/README.md";
  ASSERT_EQ(log.size(), EXPECTED_LOG_LINES) << "nestest.log 缺失或不完整";

  ember::logic::hardware::MasterScheduler scheduler;
  ember::logic::hardware::CpuAddressBus bus;
  ember::logic::hardware::WiredOrIrqLine irq;
  ember::logic::hardware::WiredOrIrqLine nmi;
  ember::logic::hardware::WiredOrRdyLine rdy;
  Ram ram;
  Rom prg(std::vector<std::uint8_t>(rom.begin() + NES_HEADER_SIZE, rom.begin() + NES_HEADER_SIZE + PRG_SIZE));
  bus.mount(0x0000, 0x1FFF, ram);
  bus.mount(0x8000, 0xFFFF, prg);
  ember::logic::hardware::Cpu6502 cpu(bus, scheduler, hw::ClockDomain{12}, irq, nmi, rdy);
  cpu.reset(true);

  for (std::size_t index = 0; index < log.size(); ++index) {
    const hw::CpuRegisters regs = cpu.registers();
    const std::string actual = describe(regs.pc, regs.a, regs.x, regs.y, regs.status, regs.sp, regs.cycles);
    ASSERT_EQ(actual, describeLogLine(log[index])) << "第 " << (index + 1) << " 行不一致，日志原文：" << log[index];
    cpu.run(1);
  }

  // 全部通过时，测试程序把结果码 00 写在 $0002 与 $0003。
  EXPECT_EQ(ram.bytes[0x0002], 0x00);
  EXPECT_EQ(ram.bytes[0x0003], 0x00);
}

}  // namespace
