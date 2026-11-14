/*
 * ------------------------------------------------------------------------
 * 名称：cpu-disassembler.hpp
 * 说明：6502 反汇编：把一条指令写成 nestest 日志风格的文本，含有效地址与当前取值的注释。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：读内存经调用方给的无副作用读取函数；芯片寄存器类地址读出 0。非官方指令的助记符按 nestest 的写法（DCP、ISB、LAX、SAX、SLO、RLA、RRA、SRE、SBC、NOP 等），
 *       其余非官方指令用通用名称（ANC、ALR、ARR、AXS、XAA、AHX、TAS、SHX、SHY、LAS、KIL）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "contracts/hardware/cpu.hpp"

namespace ember::logic::hardware {

/** 无副作用地读 CPU 总线上的一个字节。 */
using PeekFunction = std::function<std::uint8_t(std::uint16_t)>;

/** 一条指令的反汇编。 */
struct Disassembly {
  /** 指令的字节数（1 到 3）。 */
  std::uint8_t length{1};
  /** 是不是非官方指令。 */
  bool unofficial{false};
  /** 助记符与操作数，带地址与取值的注释，如 "LDA ($80,X) @ 80 = 0200 = 5A"。 */
  std::string text;
};

/**
 * 反汇编 pc 处的指令。
 * @param registers 当前的寄存器，带索引的寻址用 X、Y 算有效地址。
 */
Disassembly disassemble(std::uint16_t pc, const contracts::hardware::CpuRegisters& registers, const PeekFunction& peek);

}  // namespace ember::logic::hardware
