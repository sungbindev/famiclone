/*
 * ------------------------------------------------------------------------
 * 名称：fc-test-support.hpp
 * 说明：经典 FC 测试的公共辅助：读取测试数据文件、手写 6502 程序的小汇编器、构造 iNES 文件、装好卡带的经典 FC（FcRig）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：卡带经 iNES 解析与 mapper 插件宿主装配：官方的 nrom 插件（mapper 0，带 $6000-$7FFF 的 8K PRG RAM，blargg 的测试 ROM 把结果写在那里），
 *       需要别的 mapper 时由调用方把测试插件的包传给 FcRig（如 ppu_read_buffer 用的 mapper 3）；核心程序和测试代码里都没有 mapper 的替身实现。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "../support/plugin-test-support.hpp"
#include "bootstrap/machines/fc.hpp"
#include "contracts/mappers/mapper-cartridge.hpp"
#include "infra/mappers/plugin-registry.hpp"
#include "logic/cartridges/ines-cartridge-loader.hpp"

namespace ember::test {

namespace hw = ember::contracts::hardware;

/** 读取 core/test/data 下的文件。 */
inline std::vector<std::uint8_t> readDataFile(const std::string& relativePath) { return readBinaryFile(std::string(EMBER_TEST_DATA_DIR) + "/" + relativePath); }

/** 手写 6502 程序的小工具：只提供测试用到的几条指令。 */
class Assembler {
 public:
  explicit Assembler(std::uint16_t origin) : origin_(origin) {}

  /** 下一条指令的地址。 */
  [[nodiscard]] std::uint16_t here() const { return static_cast<std::uint16_t>(origin_ + bytes_.size()); }

  [[nodiscard]] const std::vector<std::uint8_t>& bytes() const { return bytes_; }

  Assembler& ldaImm(std::uint8_t value) { return emit({0xA9, value}); }
  Assembler& ldaAbs(std::uint16_t address) { return emit({0xAD, low(address), high(address)}); }
  Assembler& staAbs(std::uint16_t address) { return emit({0x8D, low(address), high(address)}); }
  Assembler& staAbsX(std::uint16_t address) { return emit({0x9D, low(address), high(address)}); }
  Assembler& ldxImm(std::uint8_t value) { return emit({0xA2, value}); }
  Assembler& cpxImm(std::uint8_t value) { return emit({0xE0, value}); }
  Assembler& andImm(std::uint8_t value) { return emit({0x29, value}); }
  Assembler& incAbs(std::uint16_t address) { return emit({0xEE, low(address), high(address)}); }
  Assembler& txa() { return emit({0x8A}); }
  Assembler& inx() { return emit({0xE8}); }
  Assembler& rti() { return emit({0x40}); }
  Assembler& jmp(std::uint16_t address) { return emit({0x4C, low(address), high(address)}); }
  /** 跳回自己，程序结束后空转。 */
  Assembler& jmpSelf() { return jmp(here()); }
  /** 不相等则跳到 target（只能向后跳或近距离向前）。 */
  Assembler& bne(std::uint16_t target) { return emit({0xD0, static_cast<std::uint8_t>(target - (here() + 2))}); }

  /** 经 $2006、$2007 向 PPU 地址空间写一个字节。 */
  Assembler& ppuWrite(std::uint16_t address, std::uint8_t value) {
    return ldaImm(high(address)).staAbs(0x2006).ldaImm(low(address)).staAbs(0x2006).ldaImm(value).staAbs(0x2007);
  }

 private:
  static std::uint8_t low(std::uint16_t value) { return static_cast<std::uint8_t>(value & 0xFF); }
  static std::uint8_t high(std::uint16_t value) { return static_cast<std::uint8_t>(value >> 8); }

  Assembler& emit(std::initializer_list<std::uint8_t> data) {
    bytes_.insert(bytes_.end(), data.begin(), data.end());
    return *this;
  }

  std::uint16_t origin_;
  std::vector<std::uint8_t> bytes_;
};

/**
 * 构造一个 16K PRG、8K CHR RAM 的 iNES 文件。
 * @param main 放在 $8000 的主程序，复位与 IRQ 向量指向它。
 * @param nmiHandler 放在 $8100 的 NMI 处理程序；为空时 NMI 向量也指向主程序。
 * @param vertical 为 true 时是垂直镜像，否则水平镜像。
 */
inline std::vector<std::uint8_t> makeNromFile(const Assembler& main, const Assembler* nmiHandler = nullptr, bool vertical = false) {
  std::vector<std::uint8_t> file = {'N', 'E', 'S', 0x1A, 1, 0, static_cast<std::uint8_t>(vertical ? 1 : 0), 0, 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<std::uint8_t> prg(16384, 0xEA);
  std::copy(main.bytes().begin(), main.bytes().end(), prg.begin());
  if (nmiHandler != nullptr) std::copy(nmiHandler->bytes().begin(), nmiHandler->bytes().end(), prg.begin() + 0x100);
  const std::uint16_t nmi = nmiHandler != nullptr ? 0x8100 : 0x8000;
  const std::uint16_t vectors[3] = {nmi, 0x8000, 0x8000};
  for (int i = 0; i < 3; ++i) {
    prg[0x3FFA + static_cast<std::size_t>(i) * 2] = static_cast<std::uint8_t>(vectors[i] & 0xFF);
    prg[0x3FFB + static_cast<std::size_t>(i) * 2] = static_cast<std::uint8_t>(vectors[i] >> 8);
  }
  file.insert(file.end(), prg.begin(), prg.end());
  return file;
}

/** 装好卡带的经典 FC：用 iNES 解析加 nrom 插件（及调用方给的其他测试插件）装配卡带，插入后上电。 */
struct FcRig {
  /**
   * @param file 完整的 NES 文件。
   * @param extraPlugins 除官方 nrom 以外要安装的插件包。
   */
  explicit FcRig(const std::vector<std::uint8_t>& file, const std::vector<std::vector<std::uint8_t>>& extraPlugins = {}) {
    auto registry = std::make_shared<infra::mappers::PluginRegistry>(fcRegistryOptions());
    registry->installPackage(officialPluginPackage("nrom"));
    for (const auto& package : extraPlugins) registry->installPackage(package);
    const logic::cartridges::InesCartridgeLoader loader(registry);
    cartridge = loader.assemble(loader.plan(file), machine.cartridgeIrq());
    machine.cartridgeSlot().insert(*cartridge);
    machine.powerOn();
  }

  /**
   * @param romPath 相对 core/test/data 的 ROM 路径。
   * @param extraPlugins 除官方 nrom 以外要安装的插件包。
   */
  explicit FcRig(const std::string& romPath, const std::vector<std::vector<std::uint8_t>>& extraPlugins = {}) : FcRig(readDataFile(romPath), extraPlugins) {}

  /** 从空白状态运行 frames 帧。 */
  void runFrames(int frames) {
    for (int i = 0; i < frames; ++i) machine.runFrame();
  }

  /** $6000-$7FFF 的 8K PRG RAM（卡带的存储区 prg-ram）的全部内容。 */
  [[nodiscard]] std::vector<std::uint8_t> prgRam() const {
    std::vector<std::uint8_t> bytes(8192);
    cartridge->storage("prg-ram")->readBlock(0, bytes);
    return bytes;
  }

  /** 把名称表 0 当作文字读出：每个图块号当作 ASCII 字符，行之间用换行分隔，去掉行尾空格。 */
  [[nodiscard]] std::string screenText() {
    std::string text;
    for (int row = 0; row < 30; ++row) {
      std::string line;
      for (int column = 0; column < 32; ++column) {
        const std::uint8_t tile = machine.peekPpuBus(static_cast<std::uint16_t>(0x2000 + row * 32 + column));
        line.push_back(tile >= 0x20 && tile < 0x7F ? static_cast<char>(tile) : ' ');
      }
      while (!line.empty() && line.back() == ' ') line.pop_back();
      if (!line.empty()) text += line + "\n";
    }
    return text;
  }

  ember::bootstrap::FcMachine machine;
  /** 卡带；声明在主机之后，先于主机销毁。 */
  std::unique_ptr<contracts::mappers::MapperCartridge> cartridge;
};

}  // namespace ember::test
