// ------------------------------------------------------------------------
// 名称：image-model.mjs
// 说明：Family BASIC v3 的 PRG 分析模型：读取 analysis/ 里的全部分析结果，建立“条目”（每条指令、每个跳过前缀、每段数据）并检查每个字节恰好属于代码或一段数据。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：lift.mjs（生成源码）、scan-immediates.mjs（找立即数里拆开的地址）共用；分类有问题时抛出错误并列出前 30 条。
// ------------------------------------------------------------------------
import { existsSync, readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { OPCODES } from '../../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { readAddressList, readRangeList } from './analysis-files.mjs';
import { CodeMap, OPCODE, SKIP_PREFIX, UNKNOWN } from './code-discovery.mjs';
import { readLayout, readSymbols } from './layout.mjs';
import { addressingMode } from './opcode-modes.mjs';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
export const BASE = 0x8000;
export const ROM_FILE = join(ROOT, '..', '..', '..', 'reference', 'nes-games', 'Family BASIC (Japan) (v3.0).nes');
const INES_HEADER = 16;
export const PRG_SIZE = 32768;
export const CHR_SIZE = 8192;

/**
 * 读取 ROM 里的 PRG 与 CHR。
 * @returns {{prg: Buffer, chr: Buffer}} 两段数据
 */
export function readRom() {
  const rom = readFileSync(ROM_FILE);
  return { prg: rom.subarray(INES_HEADER, INES_HEADER + PRG_SIZE), chr: rom.subarray(INES_HEADER + PRG_SIZE, INES_HEADER + PRG_SIZE + CHR_SIZE) };
}

/**
 * 四位、两位十六进制。
 * @param {number} value 数值
 * @returns {string} 大写十六进制
 */
export const hex4 = (value) => value.toString(16).toUpperCase().padStart(4, '0');
export const hex2 = (value) => value.toString(16).toUpperCase().padStart(2, '0');

/**
 * 把 addrs、words、rec 区段拆成元素。
 * @param {{start: number, end: number, kind: string, pattern: string}} range 区段
 * @returns {{start: number, size: number, kind: 'b'|'w'|'a'}[]} 元素
 */
export function dataElements(range) {
  const elements = [];
  if (range.kind === 'addrs' || range.kind === 'words') {
    for (let address = range.start; address <= range.end; address += 2) elements.push({ start: address, size: 2, kind: range.kind === 'addrs' ? 'a' : 'w' });
    return elements;
  }
  let address = range.start;
  while (address <= range.end) {
    for (const letter of range.pattern) {
      const size = letter === 'b' ? 1 : 2;
      elements.push({ start: address, size, kind: letter });
      address += size;
    }
  }
  return elements;
}

/**
 * 读取立即数地址文件。
 * @param {string} file 文件路径
 * @returns {Map<number, {half: 'lo'|'hi', target: number, offset: number}>} 指令地址到拆开的地址
 * @throws {Error} 格式不对
 */
function readImmediates(file) {
  const result = new Map();
  if (!existsSync(file)) return result;
  for (const line of readFileSync(file, 'utf8').split(/\r?\n/)) {
    const text = line.replace(/#.*$/, '').trim();
    if (text === '') continue;
    const match = /^([0-9A-Fa-f]{4})\s+(lo|hi)\s+([0-9A-Fa-f]{4})(?:\+(\d+))?$/.exec(text);
    if (!match) throw new Error(`immediates.txt：格式不对：${line}`);
    result.set(Number.parseInt(match[1], 16), { half: match[2], target: Number.parseInt(match[3], 16), offset: Number(match[4] ?? 0) });
  }
  return result;
}

/**
 * 建立分析模型。
 * @returns {object} 模型：bytes、end、inImage、map、items、owner、symbols、immediates、itemIndexAt
 * @throws {Error} 分类有问题
 */
export function loadImageModel() {
  const bytes = readRom().prg;
  const end = BASE + bytes.length;
  const analysis = (name) => join(ROOT, 'analysis', name);
  const symbols = existsSync(analysis('symbols.txt')) ? readSymbols(analysis('symbols.txt')) : new Map();
  const inImage = (address) => address >= BASE && address < end && !symbols.get(address)?.external;
  const dataRanges = readRangeList(analysis('data-ranges.txt'));
  const inDataRange = (address) => dataRanges.some((range) => address >= range.start && address <= range.end);
  const layout = readLayout(analysis('data-layout.txt'));
  const immediates = readImmediates(analysis('immediates.txt'));

  const map = new CodeMap(bytes, BASE, new Set(readAddressList(analysis('skip-prefixes.txt'))));
  for (const entry of readAddressList(analysis('trace-entries.txt'))) if (!inDataRange(entry)) map.trace([entry], { strict: false, commit: true });
  for (const entry of readAddressList(analysis('code-entries.txt'))) map.trace([entry], { strict: false, commit: true });

  const owner = new Array(bytes.length).fill(null);
  const problems = [];
  for (const range of layout) {
    for (let address = range.start; address <= range.end; address += 1) {
      const offset = address - BASE;
      if (offset < 0 || offset >= bytes.length) { problems.push(`数据区段 ${hex4(range.start)}-${hex4(range.end)} 超出映像`); break; }
      if (map.kinds[offset] !== UNKNOWN) problems.push(`$${hex4(address)} 既是代码又在数据区段 ${hex4(range.start)}-${hex4(range.end)} 里`);
      else if (owner[offset] !== null) problems.push(`$${hex4(address)} 在两个数据区段里`);
      owner[offset] = range;
    }
  }
  for (let offset = 0; offset < bytes.length; offset += 1) {
    if (map.kinds[offset] === UNKNOWN && owner[offset] === null) problems.push(`$${hex4(BASE + offset)} 没有分类`);
  }
  if (problems.length > 0) {
    const shown = [...new Set(problems)];
    throw new Error(`分类有 ${shown.length} 处问题，前 30 条：\n${shown.slice(0, 30).join('\n')}`);
  }

  /** @typedef {{start: number, size: number, type: 'code'|'skip'|'data', opcode?: number, range?: object}} Item */
  const items = [];
  for (let offset = 0; offset < bytes.length;) {
    const address = BASE + offset;
    if (map.kinds[offset] === OPCODE) {
      items.push({ start: address, size: OPCODES[bytes[offset]][1], type: 'code', opcode: bytes[offset] });
      offset += OPCODES[bytes[offset]][1];
    } else if (map.kinds[offset] === SKIP_PREFIX) {
      items.push({ start: address, size: 1, type: 'skip' });
      offset += 1;
    } else {
      const range = owner[offset];
      items.push({ start: range.start, size: range.end - range.start + 1, type: 'data', range });
      offset = range.end - BASE + 1;
    }
  }

  const itemIndexAt = (address) => {
    let low = 0;
    let high = items.length - 1;
    while (low < high) {
      const middle = (low + high + 1) >> 1;
      if (items[middle].start <= address) low = middle; else high = middle - 1;
    }
    return low;
  };
  return { bytes, end, inImage, map, items, owner, symbols, immediates, itemIndexAt, layout };
}

/**
 * 收集代码、地址表、立即数映射、符号文件引用的映像内地址。
 * @param {object} model loadImageModel 的结果
 * @returns {Map<number, Set<string>>} 地址到引用理由
 */
export function collectTargets(model) {
  const { bytes, inImage, items, symbols, immediates } = model;
  const targets = new Map();
  const refer = (address, why) => {
    if (!inImage(address)) return;
    if (!targets.has(address)) targets.set(address, new Set());
    targets.get(address).add(why);
  };
  for (const item of items) {
    if (item.type === 'code') {
      const mode = addressingMode(item.opcode);
      const offset = item.start - BASE;
      if (mode === 'rel') refer((item.start + 2 + bytes.readInt8(offset + 1)) & 0xffff, `分支 ${hex4(item.start)}`);
      else if (['abs', 'abx', 'aby', 'ind'].includes(mode)) refer(bytes.readUInt16LE(offset + 1), `操作数 ${hex4(item.start)}`);
      else if (mode === 'imm' && immediates.has(item.start)) refer(immediates.get(item.start).target, `立即数 ${hex4(item.start)}`);
    } else if (item.type === 'data' && (item.range.kind === 'addrs' || item.range.kind === 'rec')) {
      for (const element of dataElements(item.range)) if (element.kind === 'a') refer(bytes.readUInt16LE(element.start - BASE), `地址表 ${hex4(element.start)}`);
    }
  }
  for (const address of symbols.keys()) refer(address, '符号');
  return targets;
}
