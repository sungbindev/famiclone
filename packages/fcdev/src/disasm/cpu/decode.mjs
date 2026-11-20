// ------------------------------------------------------------------------
// 名称：decode.mjs
// 说明：解码一条 6502 指令：从字节序列里取出操作码、寻址方式、操作数，以及控制转移的目标地址和引用的地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：不认识的操作码与被截断的指令不抛错，返回带原因的失败结果，由调用者决定放弃这一路还是当数据处理。
// ------------------------------------------------------------------------
import { OPCODE_TABLE } from './opcodes.mjs';

/**
 * @typedef {object} DecodedInstruction
 * @property {true} ok 解码成功
 * @property {number} opcode 操作码字节
 * @property {string} mnemonic 助记符（小写）
 * @property {string} mode 寻址方式
 * @property {number} size 指令长度（含操作码）
 * @property {string} flow 流程类别
 * @property {number|null} operand 操作数的原始值：单字节为 0-255，双字节为 0-65535（相对寻址时是未做符号扩展的偏移字节）；没有操作数时为 null
 * @property {number|null} target 控制转移的目标地址：分支、JMP 绝对、JSR；其他为 null
 * @property {number|null} reference 引用的地址：零页、绝对、间接的操作数（含 JMP、JSR 的目标与间接指针）；立即数、累加器、隐含、相对为 null
 */

/**
 * @typedef {object} DecodeFailure
 * @property {false} ok 解码失败
 * @property {'illegal'|'truncated'|'outOfRange'} reason illegal 不是官方操作码；truncated 指令超出字节序列末尾；outOfRange 起点不在字节序列内
 * @property {number} [opcode] 操作码字节（illegal、truncated 时有）
 * @property {number} [size] 指令应有的长度（truncated 时有）
 */

/** 操作数是地址的寻址方式 */
const ADDRESS_MODES = new Set(['zeroPage', 'zeroPageX', 'zeroPageY', 'absolute', 'absoluteX', 'absoluteY', 'indirect', 'indexedIndirect', 'indirectIndexed']);

/**
 * 解码 bytes 里 offset 处的一条指令。
 * @param {Uint8Array} bytes 字节序列
 * @param {number} offset 指令第一个字节在序列里的位置
 * @param {number} address 这条指令的 CPU 地址，用来算分支目标
 * @returns {DecodedInstruction|DecodeFailure} 解码结果
 */
export function decode(bytes, offset, address) {
  if (!Number.isInteger(offset) || offset < 0 || offset >= bytes.length) return { ok: false, reason: 'outOfRange' };
  const opcode = bytes[offset];
  const entry = OPCODE_TABLE[opcode];
  if (entry === null) return { ok: false, reason: 'illegal', opcode };
  if (offset + entry.size > bytes.length) return { ok: false, reason: 'truncated', opcode, size: entry.size };

  let operand = null;
  if (entry.size === 2) operand = bytes[offset + 1];
  else if (entry.size === 3) operand = bytes[offset + 1] | (bytes[offset + 2] << 8);

  let target = null;
  if (entry.flow === 'branch') target = (address + 2 + ((operand << 24) >> 24)) & 0xffff;
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
