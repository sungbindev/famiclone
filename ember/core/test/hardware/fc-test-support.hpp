/*
 * ------------------------------------------------------------------------
 * 名称：fc-test-support.hpp
 * 说明：经典 FC 测试的公共辅助：测试用 NROM 卡带（按 iNES 文件头读 PRG 与 CHR，水平或垂直镜像）、读取测试数据文件、运行 ROM 的辅助函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：NROM 卡带只用于测试，核心程序里没有 mapper 的实现（mapper 将来全部是插件）；
 *       为了让 blargg 的测试 ROM 能把结果写到 $6000，卡带在 $6000-$7FFF 带 8K RAM；
 *       另支持 mapper 3（CNROM：写 $8000-$FFFF 选择 8K 的 CHR 页，无总线冲突），只为 ppu_read_buffer 测试。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "bootstrap/machines/fc.hpp"
#include "contracts/hardware/cartridge.hpp"

namespace ember::test {

namespace hw = ember::contracts::hardware;

/** 读取 core/test/data 下的文件。 */
inline std::vector<std::uint8_t> readDataFile(const std::string& relativePath) {
  std::ifstream stream(std::string(EMBER_TEST_DATA_DIR) + "/" + relativePath, std::ios::binary);
  if (!stream) throw std::runtime_error("无法打开测试数据：" + relativePath);
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/** 测试用 NROM 卡带。 */
class TestNromCartridge final : public hw::Cartridge {
 public:
  /** @param file 完整的 iNES 文件；文件头不合法或 mapper 不是 0、3 时抛出异常。 */
  explicit TestNromCartridge(const std::vector<std::uint8_t>& file) : cpuSide_(*this), ppuSide_(*this) {
    if (file.size() < 16 || file[0] != 'N' || file[1] != 'E' || file[2] != 'S' || file[3] != 0x1A) throw std::runtime_error("不是 iNES 文件");
    const std::size_t prgSize = static_cast<std::size_t>(file[4]) * 16384;
    const std::size_t chrSize = static_cast<std::size_t>(file[5]) * 8192;
    mapper_ = static_cast<std::uint8_t>((file[6] >> 4) | (file[7] & 0xF0));
    if (mapper_ != 0 && mapper_ != 3) throw std::runtime_error("测试卡带只支持 mapper 0 与 3");
    vertical_ = (file[6] & 1) != 0;
    const std::size_t trainer = (file[6] & 4) != 0 ? 512 : 0;
    if (prgSize == 0 || file.size() < 16 + trainer + prgSize + chrSize) throw std::runtime_error("iNES 文件长度不足");
    prg_.assign(file.begin() + 16 + static_cast<std::ptrdiff_t>(trainer), file.begin() + 16 + static_cast<std::ptrdiff_t>(trainer + prgSize));
    if (chrSize == 0) {
      chr_.assign(8192, 0);
      chrIsRam_ = true;
    } else {
      chr_.assign(file.begin() + 16 + static_cast<std::ptrdiff_t>(trainer + prgSize), file.begin() + 16 + static_cast<std::ptrdiff_t>(trainer + prgSize + chrSize));
    }
    prgRam_.assign(8192, 0);
  }

  hw::BusDevice& cpuSide() override { return cpuSide_; }
  hw::BusDevice& ppuSide() override { return ppuSide_; }

  [[nodiscard]] hw::CiramControl ciramControl(std::uint16_t ppuAddress) const override { return hw::CiramControl{true, vertical_ ? (ppuAddress & 0x400) != 0 : (ppuAddress & 0x800) != 0}; }

  void reset(bool power) override {
    if (power) {
      std::fill(prgRam_.begin(), prgRam_.end(), std::uint8_t{0});
      chrBank_ = 0;
    }
  }

  /** $6000-$7FFF 的 8K RAM，测试读取结果用。 */
  [[nodiscard]] std::span<const std::uint8_t> prgRam() const { return prgRam_; }

 private:
  class CpuSide final : public hw::BusDevice {
   public:
    explicit CpuSide(TestNromCartridge& owner) : owner_(owner) {}

    hw::BusRead read(std::uint16_t address) override {
      if (address >= 0x8000) return hw::driveBus(owner_.prg_[(address - 0x8000U) % owner_.prg_.size()]);
      if (address >= 0x6000) return hw::driveBus(owner_.prgRam_[address - 0x6000U]);
      return hw::releaseBus();
    }

    void write(std::uint16_t address, std::uint8_t value) override {
      if (address >= 0x8000) {
        if (owner_.mapper_ == 3) owner_.chrBank_ = value;
      } else if (address >= 0x6000) {
        owner_.prgRam_[address - 0x6000U] = value;
      }
    }

   private:
    TestNromCartridge& owner_;
  };

  class PpuSide final : public hw::BusDevice {
   public:
    explicit PpuSide(TestNromCartridge& owner) : owner_(owner) {}

    hw::BusRead read(std::uint16_t address) override {
      if (address >= 0x2000) return hw::releaseBus();
      return hw::driveBus(owner_.chr_[owner_.chrOffset(address)]);
    }

    void write(std::uint16_t address, std::uint8_t value) override {
      if (address < 0x2000 && owner_.chrIsRam_) owner_.chr_[owner_.chrOffset(address)] = value;
    }

   private:
    TestNromCartridge& owner_;
  };

  [[nodiscard]] std::size_t chrOffset(std::uint16_t address) const { return (static_cast<std::size_t>(chrBank_) * 8192 + address) % chr_.size(); }

  std::vector<std::uint8_t> prg_;
  std::vector<std::uint8_t> chr_;
  std::vector<std::uint8_t> prgRam_;
  std::uint8_t mapper_{0};
  std::uint8_t chrBank_{0};
  bool vertical_{false};
  bool chrIsRam_{false};
  CpuSide cpuSide_;
  PpuSide ppuSide_;
};

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

/** 装好卡带的经典 FC：上电前先插卡带。 */
struct FcRig {
  explicit FcRig(const std::vector<std::uint8_t>& file) : cartridge(file) {
    machine.cartridgeSlot().insert(cartridge);
    machine.powerOn();
  }

  /** @param romPath 相对 core/test/data 的 ROM 路径。 */
  explicit FcRig(const std::string& romPath) : FcRig(readDataFile(romPath)) {}

  /** 从空白状态运行 frames 帧。 */
  void runFrames(int frames) {
    for (int i = 0; i < frames; ++i) machine.runFrame();
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
  TestNromCartridge cartridge;
};

}  // namespace ember::test
