/*
 * ------------------------------------------------------------------------
 * 名称：cec1-board.hpp
 * 说明：中华学习机 CEC-I 的主板：64K 主存、语言卡的 16K 存储、两块 32K 的 ROM（U7 与 U35），
 *       以及 $C000-$C0FF 的软开关、内置软驱接口与按 ROM 自检程序推断出的 $C0B0 寄存器；
 *       整个 CPU 地址空间都由本板译码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：ROM 文件是 64K：前 32K 是 U7（低 16K 是 LOGO 语言，高 16K 对应 $C000-$FFFF：
 *       $C100-$CFFF 是内部 ROM、$D000-$FFFF 是监控与 CEC-BASIC），
 *       后 32K 是 U35（低 16K 是汉字表，高 16K 对应 $C000-$FFFF：$C300 与 $C600 是槽 ROM、
 *       $D000-$FFFF 是汉字程序）；
 *       CPU 读的译码（见 locateRead）：
 *         $0000-$7FFF 主存；$8000-$BFFF 主存，辅存读开关（$C002/$C003）为 1 时是 ROM：
 *         $C0B0 的位 5 为 1 读 U7 低 16K（LOGO），为 0 读 U35 低 16K（汉字表），
 *         $4000-$7FFF 在辅存读开关为 1 时读到空座（0）；
 *         $C000-$C0FF 软开关与 I/O；$C100-$CFFF：内部 ROM 开关（$C006/$C007）为 1 时读 U7 高 16K，
 *         否则读 U35 高 16K（槽 ROM），其中 $C3xx 在槽 3 开关（$C00A/$C00B）为 0 时也读 U7；
 *         $D000-$FFFF：辅助零页开关（$C008/$C009）为 1 时读 U35 高 16K（汉字程序），
 *         否则语言卡读 RAM 时读语言卡存储，否则读 U7 高 16K；
 *       写：$0000-$BFFF 总是写主存（辅存写开关只记录状态）；
 *       $D000-$FFFF 语言卡写允许时写语言卡存储，其余写入忽略；
 *       语言卡（$C080-$C08F，读与写都算访问）：A3 为 0 选 bank 2（$D000-$DFFF），为 1 选 bank 1；
 *       A1A0 为 00 读 RAM 写保护、01 读 ROM、连续两次访问写允许、10 读 ROM 写保护、11 读 RAM、
 *       连续两次访问写允许；
 *       软开关：$C000-$C00F 写：偶数地址关、奇数地址开：80 列存储、辅存读、辅存写、内部 ROM、
 *       辅助零页、槽 3 ROM、80 列显示、备用字符集；
 *       $C000 读键盘，$C010 读写清键盘标志，$C011-$C01F 读状态（位 7）；$C020 磁带输出翻转，
 *       $C030 扬声器翻转，$C050-$C057 显示开关，
 *       $C060 磁带输入、$C061-$C063 按键 0 到 2（按键 2 是 Shift）、
 *       $C064-$C067 模拟手柄 0 到 3 的计时，$C070 触发模拟手柄计时；
 *       $C0E0-$C0EF 内置软驱接口（槽 6）；$C0AE、$C0AF 内置串口（槽 2，状态位 1 恒为发送就绪，
 *       读不到数据）；$C090、$C1C1 打印口（没有接打印机，数据丢弃）；
 *       $C0B0 寄存器：读不驱动数据线；写入的值整个保存，位 5 选 LOGO 或汉字表（见上），
 *       位 4 在复位后为 $10 的含义没有查明，
 *       低 4 位在辅存读开关为 1 时选 $4000-$7FFF 的扩展 ROM 页（本机没有，读 0）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "contracts/hardware/bus-device.hpp"
#include "contracts/hardware/clock.hpp"
#include "contracts/hardware/tape-deck.hpp"
#include "logic/hardware/disk2/disk2.hpp"
#include "logic/hardware/keyboard-apple2/keyboard-apple2.hpp"
#include "logic/hardware/speaker-1bit/speaker-1bit.hpp"
#include "logic/hardware/video-apple2/video-apple2.hpp"

namespace ember::bootstrap {

/** 中华学习机 CEC-I 的主板。 */
class Cec1Board final : public contracts::hardware::BusDevice, public logic::hardware::Apple2VideoSource {
 public:
  /** ROM 文件的大小：U7 与 U35 各 32K。 */
  static constexpr std::size_t ROM_SIZE = 64 * 1024;
  /**
   * 可选的汉字字库 ROM（16 个 bank，每个 16K）：接在 ROM 文件的后面时，
   * 文件是 ROM_SIZE + LIBRARY_SIZE。
   */
  static constexpr std::size_t LIBRARY_SIZE = 256 * 1024;
  static constexpr std::size_t LIBRARY_BANK_SIZE = 16 * 1024;
  /** 主存的大小。 */
  static constexpr std::size_t RAM_SIZE = 64 * 1024;
  /** 语言卡存储的大小：bank 1 的 4K、bank 2 的 4K 与 $E000-$FFFF 的 8K。 */
  static constexpr std::size_t LANGUAGE_CARD_SIZE = 16 * 1024;
  /** ROM 里 U7 低 16K、U7 高 16K、U35 低 16K、U35 高 16K 的起点。 */
  static constexpr std::size_t U7_LOW = 0x0000;
  static constexpr std::size_t U7_HIGH = 0x4000;
  static constexpr std::size_t U35_LOW = 0x8000;
  static constexpr std::size_t U35_HIGH = 0xC000;

  /** 软开关与寄存器的状态。 */
  struct Switches {
    bool text{true};
    bool mixed{false};
    bool page2{false};
    bool hires{false};
    bool store80{false};
    bool ramRead{false};
    bool ramWrite{false};
    bool internalRom{false};
    bool altZeroPage{false};
    bool slotThreeRom{false};
    bool column80{false};
    bool altCharset{false};
    /** 语言卡：读 RAM、写允许、选 bank 2、已有一次奇数地址访问。 */
    bool lcReadRam{false};
    bool lcWriteRam{false};
    bool lcBank2{true};
    bool lcPreWrite{false};
    /** 四个信号输出。 */
    std::array<bool, 4> annunciators{};
    /** $C0B0 寄存器。 */
    std::uint8_t aux{0};
  };

  /** CPU 地址所在的位置，调试用。 */
  struct Location {
    enum class Kind : std::uint8_t { Ram, LanguageCard, Rom, Register, Empty, None };
    Kind kind{Kind::None};
    /** 存储区里的偏移（Rom 是 ROM 文件里的偏移，Register 是 $C000 起的偏移）。 */
    std::size_t offset{0};
  };

  /**
   * @param rom ROM 的内容，必须是 ROM_SIZE 字节（U7 在前、U35 在后，不含文件头），
   * 或后面再接 LIBRARY_SIZE 字节的汉字字库。
   * @param clock 主时钟：扬声器与模拟手柄的计时用它。
   * @param keyboard 键盘。
   * @param disk 内置软驱接口（槽 6）。
   * @param speaker 扬声器。
   * @param tape 磁带线（$C020 输出、$C060 输入）。
   * @param cpuTicksPerCycle 每个 CPU 周期的主时钟周期数（模拟手柄的计时按 CPU 周期换算）。
   * @throws shared::CoreError hardware.invalidArgument：ROM 大小不对。
   */
  Cec1Board(std::vector<std::uint8_t> rom, const contracts::hardware::Clock& clock, logic::hardware::KeyboardApple2& keyboard, logic::hardware::Disk2& disk, logic::hardware::Speaker1Bit& speaker,
            contracts::hardware::TapeLine& tape, contracts::hardware::MasterTime cpuTicksPerCycle);

  /**
   * 复位。
   * @param power 上电时主存与语言卡存储清零，软开关与寄存器回到默认值；
   * 软复位只让软开关与寄存器回到默认值。
   */
  void reset(bool power);

  // BusDevice
  contracts::hardware::BusRead read(std::uint16_t address) override;
  void write(std::uint16_t address, std::uint8_t value) override;
  contracts::hardware::BusRead peek(std::uint16_t address) override;

  // Apple2VideoSource
  [[nodiscard]] logic::hardware::Apple2VideoMode videoMode() const override;
  [[nodiscard]] std::uint8_t readVideoByte(std::uint16_t address) const override { return ram_[address]; }

  /**
   * 模拟手柄与按键：按手柄的按键掩码设置（方向键对应模拟手柄 0 的横向与手柄 1 的纵向，
   * A 与 B 是按键 0 与 1）。
   */
  void setPaddles(std::uint8_t buttons);

  /** CPU 读这个地址时读到的位置。 */
  [[nodiscard]] Location locateRead(std::uint16_t address) const;

  [[nodiscard]] const Switches& switches() const { return switches_; }
  [[nodiscard]] const std::vector<std::uint8_t>& rom() const { return rom_; }
  [[nodiscard]] std::vector<std::uint8_t>& ram() { return ram_; }
  [[nodiscard]] const std::vector<std::uint8_t>& ram() const { return ram_; }
  [[nodiscard]] std::vector<std::uint8_t>& languageCard() { return languageCard_; }
  [[nodiscard]] const std::vector<std::uint8_t>& languageCard() const { return languageCard_; }

 private:
  /** 读 $C000-$C0FF；side 为假时不产生副作用。 */
  contracts::hardware::BusRead readIo(std::uint8_t reg, bool side);
  void writeIo(std::uint8_t reg, std::uint8_t value);
  /** $C000-$C00F 的写。 */
  void writeSwitch(std::uint8_t reg);
  /** 语言卡的一次访问（读或写）。 */
  void accessLanguageCard(std::uint8_t reg);
  /** 显示开关（$C050-$C057）的访问。 */
  void accessVideoSwitch(std::uint8_t reg);
  /** $C011-$C01F 的状态位 7。 */
  [[nodiscard]] bool statusFlag(std::uint8_t reg) const;
  /** 语言卡存储里 CPU 地址（$D000-$FFFF）对应的偏移。 */
  [[nodiscard]] std::size_t languageCardOffset(std::uint16_t address) const;
  /** 模拟手柄 n 的计时是否还没结束。 */
  [[nodiscard]] bool paddleTiming(unsigned paddle) const;

  const contracts::hardware::Clock& clock_;
  logic::hardware::KeyboardApple2& keyboard_;
  logic::hardware::Disk2& disk_;
  logic::hardware::Speaker1Bit& speaker_;
  contracts::hardware::TapeLine& tape_;
  contracts::hardware::MasterTime cpuTicksPerCycle_;

  std::vector<std::uint8_t> rom_;
  std::vector<std::uint8_t> ram_;
  std::vector<std::uint8_t> languageCard_;
  Switches switches_;

  /** 磁带输出线当前的电平。 */
  bool tapeOutput_{false};
  /** 手柄按键：按键 0 与 1，以及四个模拟手柄的值（0 到 255）。 */
  bool button0_{false};
  bool button1_{false};
  std::array<std::uint8_t, 4> paddleValues_{128, 128, 128, 128};
  /** 最近一次触发模拟手柄计时的时间。 */
  contracts::hardware::MasterTime paddleTrigger_{-1'000'000'000};
};

}  // namespace ember::bootstrap
