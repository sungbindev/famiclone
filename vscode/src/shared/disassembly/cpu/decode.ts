// ------------------------------------------------------------------------
// 名称：decode.ts
// 说明：解码一条 6502 指令：从字节序列里取出操作码、寻址方式、操作数，以及控制转移的目标地址和引用
//       的地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：不认识的操作码与被截断的指令不抛错，返回带原因的失败结果，由调用者决定放弃这一路还是当数据
//       处理。
// ------------------------------------------------------------------------
import { OPCODE_TABLE } from './opcodes';
import type { AddressingMode, FlowKind } from './opcodes';

export interface DecodedInstruction {
  /** 解码成功 */
  ok: true;
  /** 操作码字节 */
  opcode: number;
  /** 助记符（小写） */
  mnemonic: string;
  /** 寻址方式 */
  mode: AddressingMode;
  /** 指令长度（含操作码） */
  size: number;
  /** 流程类别 */
  flow: FlowKind;
  /**
   * 操作数的原始值：单字节为 0-255，双字节为 0-65535（相对寻址时是未做符号扩展的偏移字节）；没有操
   * 作数时为 null
   */
  operand: number|null;
  /** 控制转移的目标地址：分支、JMP 绝对、JSR；其他为 null */
  target: number|null;
  /**
   * 引用的地址：零页、绝对、间接的操作数（含 JMP、JSR 的目标与间接指针）；立即数、累加器、隐含、相
   * 对为 null
   */
  reference: number|null;
}

export interface DecodeFailure {
  /** 解码失败 */
  ok: false;
  /** illegal 不是官方操作码；truncated 指令超出字节序列末尾；outOfRange 起点不在字节序列内 */
  reason: 'illegal'|'truncated'|'outOfRange';
  /** 操作码字节（illegal、truncated 时有） */
  opcode?: number;
  /** 指令应有的长度（truncated 时有） */
  size?: number;
}

/** 操作数是地址的寻址方式 */
const ADDRESS_MODES = new Set(['zeroPage', 'zeroPageX', 'zeroPageY', 'absolute', 'absoluteX', 'absoluteY', 'indirect', 'indexedIndirect', 'indirectIndexed']);

/**
 * 解码 bytes 里 offset 处的一条指令。
 * @param bytes 字节序列
 * @param offset 指令第一个字节在序列里的位置
 * @param address 这条指令的 CPU 地址，用来算分支目标
 * @returns 解码结果
 */
export function decode(bytes: Uint8Array, offset: number, address: number): DecodedInstruction|DecodeFailure {
  if (!Number.isInteger(offset) || offset < 0 || offset >= bytes.length) return { ok: false, reason: 'outOfRange' };
  const opcode = bytes[offset] ?? 0;
  const entry = OPCODE_TABLE[opcode] ?? null;
  if (entry === null) return { ok: false, reason: 'illegal', opcode };
  if (offset + entry.size > bytes.length) return { ok: false, reason: 'truncated', opcode, size: entry.size };

  let operand: number | null = null;
  if (entry.size === 2) operand = bytes[offset + 1] ?? 0;
  else if (entry.size === 3) operand = (bytes[offset + 1] ?? 0) | ((bytes[offset + 2] ?? 0) << 8);

  let target: number | null = null;
  if (entry.flow === 'branch') target = (address + 2 + (((operand ?? 0) << 24) >> 24)) & 0xffff;
  else if (entry.flow === 'jump' || entry.flow === 'call') target = operand;

  return {
    ok: true,
    opcode,
    mnemonic: entry.mnemonic,
    mode: entry.mode,
    size: entry.size,
    flow: entry.flow,
    operand,
    target,
    reference: ADDRESS_MODES.has(entry.mode) ? operand : null,
  };
}
