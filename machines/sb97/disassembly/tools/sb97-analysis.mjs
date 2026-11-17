// ------------------------------------------------------------------------
// 名称：sb97-analysis.mjs
// 说明：小霸王 97 BIOS 的静态分析：递归下降找代码、统计对 $2000-$7FFF 寄存器的访问、
//       找出写 $5000 前装入的页号立即数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：只看指令的静态操作数，间接寻址与跳转表里的目标需要由调用方给出入口。
// ------------------------------------------------------------------------
import { decodeInstruction, endsFlow, formatInstruction } from '../../../sb2000/disassembly/tools/disasm.mjs';
import { PAGE_SIZE } from './sb97-rom.mjs';

const READ_MNEMONICS = new Set(['LDA', 'LDX', 'LDY', 'BIT', 'CMP', 'CPX', 'CPY', 'ADC', 'SBC', 'AND', 'ORA', 'EOR', 'LAX']);
const WRITE_MNEMONICS = new Set(['STA', 'STX', 'STY', 'SAX']);
const MODIFY_MNEMONICS = new Set(['INC', 'DEC', 'ASL', 'LSR', 'ROL', 'ROR']);
const ADDRESSED_MODES = new Set(['abs', 'abx', 'aby']);

/**
 * 指令对内存操作数的访问方式。
 * @param {string} mnemonic 助记符
 * @returns {'r'|'w'|'rw'|null} 不访问内存时为 null
 */
export function accessKind(mnemonic) {
  if (READ_MNEMONICS.has(mnemonic)) return 'r';
  if (WRITE_MNEMONICS.has(mnemonic)) return 'w';
  if (MODIFY_MNEMONICS.has(mnemonic)) return 'rw';
  return null;
}

/**
 * 从入口开始递归下降，收集可达的指令。
 * @param {(address: number) => number} read 按地址读字节
 * @param {number[]} entries 入口地址
 * @param {(address: number) => boolean} inRange 地址是否在要分析的范围内
 * @returns {Map<number, import('../../../sb2000/disassembly/tools/disasm.mjs').Instruction>} 地址到指令
 */
export function traceCode(read, entries, inRange) {
  const found = new Map();
  const pending = [...entries];
  while (pending.length > 0) {
    let pc = pending.pop();
    while (inRange(pc) && !found.has(pc)) {
      const ins = decodeInstruction(read, pc);
      found.set(pc, ins);
      if (ins.target !== null && inRange(ins.target)) pending.push(ins.target);
      if (endsFlow(ins)) break;
      pc = (pc + ins.length) & 0xffff;
    }
  }
  return found;
}

/**
 * 统计对 $2000-$7FFF 的寄存器类访问，只统计官方指令的绝对寻址。
 * @param {Uint8Array[]} pages 要线性扫描的页（按 $8000 基址解码）
 * @param {number[]} pageNumbers 与 pages 对应的页号
 * @returns {Map<number, {read: number, write: number, pages: Set<number>}>} 地址到访问统计
 */
export function scanRegisterAccess(pages, pageNumbers) {
  const result = new Map();
  pages.forEach((page, index) => {
    const read = (address) => page[address & 0x3fff];
    for (let pc = 0; pc < PAGE_SIZE; ) {
      const ins = decodeInstruction(read, pc);
      pc += ins.length;
      if (!ins.info.official || !ADDRESSED_MODES.has(ins.info.mode)) continue;
      const kind = accessKind(ins.info.mnemonic);
      if (kind === null || ins.operand < 0x2000 || ins.operand >= 0x8000) continue;
      const entry = result.get(ins.operand) ?? { read: 0, write: 0, pages: new Set() };
      if (kind !== 'w') entry.read += 1;
      if (kind !== 'r') entry.write += 1;
      entry.pages.add(pageNumbers[index]);
      result.set(ins.operand, entry);
    }
  });
  return result;
}

/**
 * 找出写 $5000 之前 3 条指令内装入的立即数（页号）。
 * @param {Uint8Array[]} pages 要线性扫描的页
 * @param {number[]} pageNumbers 与 pages 对应的页号
 * @returns {Map<number, Set<number>>} 页号立即数到出现它的页
 */
export function findBankImmediates(pages, pageNumbers) {
  const result = new Map();
  pages.forEach((page, index) => {
    const read = (address) => page[address & 0x3fff];
    const recent = [];
    for (let pc = 0; pc < PAGE_SIZE; ) {
      const ins = decodeInstruction(read, pc);
      pc += ins.length;
      if (ins.info.official && ins.info.mnemonic === 'STA' && ins.info.mode === 'abs' && ins.operand === 0x5000) {
        const load = [...recent].reverse().find((item) => item.info.mnemonic === 'LDA');
        if (load?.info.mode === 'imm') {
          const pagesOfValue = result.get(load.operand) ?? new Set();
          pagesOfValue.add(pageNumbers[index]);
          result.set(load.operand, pagesOfValue);
        }
      }
      recent.push(ins);
      if (recent.length > 3) recent.shift();
    }
  });
  return result;
}

/**
 * 把递归下降的结果和其余字节排成文本清单：指令一行一条，没走到的连续字节按 .byte 输出。
 * @param {(address: number) => number} read 按地址读字节
 * @param {Map<number, import('../../../sb2000/disassembly/tools/disasm.mjs').Instruction>} code 可达指令
 * @param {number} start 起始地址
 * @param {number} size 字节数
 * @returns {string[]} 清单行
 */
export function formatListing(read, code, start, size) {
  const hex2 = (value) => value.toString(16).toUpperCase().padStart(2, '0');
  const hex4 = (value) => value.toString(16).toUpperCase().padStart(4, '0');
  const lines = [];
  const end = start + size;
  for (let pc = start; pc < end; ) {
    const ins = code.get(pc);
    if (ins) {
      const bytes = Array.from({ length: ins.length }, (_, k) => hex2(read(pc + k))).join(' ');
      lines.push(`${hex4(pc)}  ${bytes.padEnd(8)}  ${formatInstruction(ins)}`);
      pc += ins.length;
      continue;
    }
    const chunk = [];
    while (pc < end && !code.has(pc) && chunk.length < 8) {
      chunk.push(hex2(read(pc)));
      pc += 1;
    }
    lines.push(`${hex4(pc - chunk.length)}  .byte ${chunk.map((value) => `$${value}`).join(',')}`);
  }
  return lines;
}
