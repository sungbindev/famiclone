// ------------------------------------------------------------------------
// 名称：sb97-rom.mjs
// 说明：小霸王 97 BIOS 文件（带 iNES/NES 2.0 文件头）的解析、分页视图与代码页判别。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：分页模型是由反汇编推断出来的假设（第 0 页固定在 $C000-$FFFF，其余页经 $5000
//       选进 $8000-$BFFF），尚未经真机核对，见 docs/README.md。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { decodeInstruction } from '../../../sb2000/disassembly/tools/disasm.mjs';

/** 一页 PRG 的字节数。 */
export const PAGE_SIZE = 0x4000;

/** 固定在 $C000-$FFFF 的页号（假设）。 */
export const FIXED_PAGE = 0;

/** 可换页窗口的起始地址（假设，由 $5000 选页）。 */
export const WINDOW_BASE = 0x8000;

/**
 * @typedef {object} RomInfo
 * @property {{nes2: boolean, prgPages: number, chrPages: number, mapper: number, submapper: number, verticalMirroring: boolean, battery: boolean, chrRamSize: number}} header 文件头信息
 * @property {Uint8Array[]} pages 按 16KB 切开的 PRG
 */

/**
 * 解析 NES 文件。
 * @param {Uint8Array} bytes 文件内容
 * @returns {RomInfo}
 */
export function parseRom(bytes) {
  if (bytes.length < 16 || bytes[0] !== 0x4e || bytes[1] !== 0x45 || bytes[2] !== 0x53 || bytes[3] !== 0x1a) {
    throw new Error('不是 iNES 文件：缺少文件头标记');
  }
  const nes2 = (bytes[7] & 0x0c) === 0x08;
  const prgPages = bytes[4] | (nes2 ? (bytes[9] & 0x0f) << 8 : 0);
  const mapper = (bytes[6] >> 4) | (bytes[7] & 0xf0) | (nes2 ? (bytes[8] & 0x0f) << 8 : 0);
  const chrRamShift = nes2 ? bytes[11] & 0x0f : 0;
  const header = {
    nes2,
    prgPages,
    chrPages: bytes[5],
    mapper,
    submapper: nes2 ? bytes[8] >> 4 : 0,
    verticalMirroring: (bytes[6] & 1) !== 0,
    battery: (bytes[6] & 2) !== 0,
    chrRamSize: chrRamShift === 0 ? 0 : 64 << chrRamShift,
  };
  const start = 16 + ((bytes[6] & 4) !== 0 ? 512 : 0);
  const end = start + prgPages * PAGE_SIZE;
  if (bytes.length < end) throw new Error(`文件长度不够：文件头声明 ${prgPages} 页，实际只有 ${bytes.length - start} 字节`);
  const pages = [];
  for (let i = 0; i < prgPages; i += 1) pages.push(bytes.subarray(start + i * PAGE_SIZE, start + (i + 1) * PAGE_SIZE));
  return { header, pages };
}

/**
 * 读取并解析 NES 文件。
 * @param {string} filePath 文件路径
 * @returns {RomInfo}
 */
export function loadRom(filePath) {
  return parseRom(readFileSync(filePath));
}

/**
 * 按“固定页加一个可换页”的假设构造 CPU 视图。
 * @param {RomInfo} rom 解析结果
 * @param {number} windowPage 当前映射到 $8000-$BFFF 的页号
 * @returns {{read: (address: number) => number, isRom: (address: number) => boolean}}
 */
export function createCpuView(rom, windowPage) {
  return {
    read(address) {
      const value = address & 0xffff;
      if (value >= 0xc000) return rom.pages[FIXED_PAGE][value - 0xc000];
      if (value >= WINDOW_BASE) return rom.pages[windowPage][value - WINDOW_BASE];
      return 0;
    },
    isRom: (address) => (address & 0xffff) >= WINDOW_BASE,
  };
}

/**
 * 读取一页末尾的三个向量（NMI、RESET、IRQ），按这一页映射在 $C000 的假设。
 * @param {Uint8Array} page 一页的数据
 * @returns {{nmi: number, reset: number, irq: number}}
 */
export function pageVectors(page) {
  const word = (offset) => page[offset] | (page[offset + 1] << 8);
  return { nmi: word(0x3ffa), reset: word(0x3ffc), irq: word(0x3ffe) };
}

/** 疑似代码页的判别门限：官方指令占比、跳转与返回指令密度、跳转目标落在 $8000 以上的占比。 */
export const CODE_THRESHOLDS = Object.freeze({ official: 0.85, jump: 0.05, rts: 0.025, inRange: 0.8 });

/**
 * 判别一页的内容：全空、疑似代码、其他（数据、字库、压缩数据等）。
 * 从页首按指令长度线性解码，用官方指令占比、JMP/JSR 与 RTS 的密度、跳转目标是否落在
 * $8000 以上这几项指标判断；点阵字库等数据也可能被误判为疑似代码，需要人工确认。
 * @param {Uint8Array} page 一页的数据
 * @returns {{kind: 'empty'|'code-like'|'other', fillRatio: number, officialRatio: number, jumpRatio: number, rtsRatio: number, inRangeRatio: number}}
 */
export function classifyPage(page) {
  const counts = new Map();
  for (const value of page) counts.set(value, (counts.get(value) ?? 0) + 1);
  const fillRatio = Math.max(...counts.values()) / page.length;
  const read = (address) => page[address & 0x3fff];
  let total = 0;
  let official = 0;
  let jumps = 0;
  let inRange = 0;
  let returns = 0;
  for (let pc = 0; pc < PAGE_SIZE; ) {
    const ins = decodeInstruction(read, pc);
    total += 1;
    if (ins.info.official) official += 1;
    if (ins.info.mnemonic === 'RTS') returns += 1;
    if (ins.target !== null && ins.info.mode === 'abs') {
      jumps += 1;
      if (ins.target >= WINDOW_BASE) inRange += 1;
    }
    pc += ins.length;
  }
  const officialRatio = official / total;
  const jumpRatio = jumps / total;
  const rtsRatio = returns / total;
  const inRangeRatio = jumps === 0 ? 0 : inRange / jumps;
  let kind = 'other';
  if (fillRatio >= 0.98) kind = 'empty';
  else if (
    officialRatio >= CODE_THRESHOLDS.official &&
    jumpRatio >= CODE_THRESHOLDS.jump &&
    rtsRatio >= CODE_THRESHOLDS.rts &&
    inRangeRatio >= CODE_THRESHOLDS.inRange
  ) {
    kind = 'code-like';
  }
  return { kind, fillRatio, officialRatio, jumpRatio, rtsRatio, inRangeRatio };
}
