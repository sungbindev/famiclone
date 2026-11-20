// ------------------------------------------------------------------------
// 名称：opcode-modes.mjs
// 说明：6502 官方指令的寻址方式与 ca65 的操作数写法。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：寻址方式按操作码的 aaabbbcc 规律推出，只覆盖官方指令；助记符与长度来自 fcdev 的指令表。
// ------------------------------------------------------------------------
import { OPCODES } from './opcodes.mjs';

/**
 * 取出操作码的寻址方式。
 * @param {number} opcode 操作码
 * @returns {'imp'|'acc'|'imm'|'zp'|'zpx'|'zpy'|'abs'|'abx'|'aby'|'ind'|'izx'|'izy'|'rel'} 寻址方式
 * @throws {Error} 不是官方指令
 */
export function addressingMode(opcode) {
  const entry = OPCODES[opcode];
  if (entry === null) throw new Error(`操作码 $${opcode.toString(16)} 不是官方指令`);
  const [mnemonic, size, flow] = entry;
  if (flow === 'branch') return 'rel';
  if (opcode === 0x20 || opcode === 0x4c) return 'abs';
  if (opcode === 0x6c) return 'ind';
  if (size === 1) return ['asl', 'lsr', 'rol', 'ror'].includes(mnemonic) ? 'acc' : 'imp';
  const group = opcode & 3;
  const mode = (opcode >> 2) & 7;
  if (group === 1) return ['izx', 'zp', 'imm', 'abs', 'izy', 'zpx', 'aby', 'abx'][mode];
  const table = group === 2 ? ['imm', 'zp', 'acc', 'abs', null, 'zpx', null, 'abx'] : ['imm', 'zp', null, 'abs', null, 'zpx', null, 'abx'];
  let result = table[mode];
  if (group === 2 && (mnemonic === 'ldx' || mnemonic === 'stx')) {
    if (result === 'zpx') result = 'zpy';
    if (result === 'abx') result = 'aby';
  }
  return result;
}

/**
 * 写出一条指令的 ca65 文字。
 * @param {number} opcode 操作码
 * @param {string} operand 已经格式化好的操作数表达式（没有操作数的指令传空串）
 * @param {boolean} forceAbsolute 绝对寻址的操作数是 $00-$FF 时要加 a: 前缀，避免被汇编成零页指令
 * @returns {string} 如 lda     $1234,x
 */
export function formatInstruction(opcode, operand, forceAbsolute) {
  const [mnemonic] = OPCODES[opcode];
  const mode = addressingMode(opcode);
  const prefix = forceAbsolute && ['abs', 'abx', 'aby'].includes(mode) ? 'a:' : '';
  let text;
  switch (mode) {
    case 'imp': text = ''; break;
    case 'acc': text = 'a'; break;
    case 'imm': text = `#${operand}`; break;
    case 'zp': case 'abs': case 'rel': text = `${prefix}${operand}`; break;
    case 'zpx': case 'abx': text = `${prefix}${operand},x`; break;
    case 'zpy': case 'aby': text = `${prefix}${operand},y`; break;
    case 'ind': text = `(${operand})`; break;
    case 'izx': text = `(${operand},x)`; break;
    case 'izy': text = `(${operand}),y`; break;
    default: throw new Error(`未知寻址方式 ${mode}`);
  }
  return text === '' ? mnemonic : `${mnemonic.padEnd(8)}${text}`;
}
