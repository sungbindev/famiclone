// ------------------------------------------------------------------------
// 名称：image-model.mjs
// 说明：Family BASIC v3 的 PRG 分析模型：读取 analysis/ 里的全部分析结果，建立“条目”（每条指令、每个跳过前缀、每段数据）并检查每个字节恰好属于代码或一段数据。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：lift.mjs（生成源码）、stats.mjs、inventory.mjs 等共用；分类有问题时抛出错误并列出前 30 条。
// ------------------------------------------------------------------------
import { existsSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { OPCODES } from '../../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { OPCODE, SKIP_PREFIX, UNKNOWN } from './code-discovery.mjs';
import { dataElements, readSymbols } from './layout.mjs';
import { addressingMode } from './opcode-modes.mjs';
import { BASE, CHR_SIZE, PRG_SIZE, ROM_FILE, ROOT, readRom } from './rom.mjs';
import { buildSeedMap } from './seed-map.mjs';

export { BASE, CHR_SIZE, PRG_SIZE, ROM_FILE, ROOT, dataElements, readRom };

/**
 * 四位、两位十六进制。
 * @param {number} value 数值
 * @returns {string} 大写十六进制
 */
export const hex4 = (value) => value.toString(16).toUpperCase().padStart(4, '0');
export const hex2 = (value) => value.toString(16).toUpperCase().padStart(2, '0');

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
  const immediates = readImmediates(analysis('immediates.txt'));
  const { map, layout } = buildSeedMap();

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
  // 没有写进布局的未知字节按“未细分的数据”处理（逐字节原样输出）；它们里的地址只有写进布局或立即数文件才会成为标签
  for (let offset = 0; offset < bytes.length;) {
    if (map.kinds[offset] !== UNKNOWN || owner[offset] !== null) { offset += 1; continue; }
    let last = offset;
    while (last + 1 < bytes.length && map.kinds[last + 1] === UNKNOWN && owner[last + 1] === null) last += 1;
    const range = { start: BASE + offset, end: BASE + last, kind: 'bytes', pattern: '', note: '未细分的数据', implicit: true };
    layout.push(range);
    for (let index = offset; index <= last; index += 1) owner[index] = range;
    offset = last + 1;
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
