// ------------------------------------------------------------------------
// 名称：lift.mjs
// 说明：把 Family BASIC v3 的 PRG 还原成可重定位的 ca65 源码 reference/fbasic-nes.s：代码、数据、地址表都用标签，汇编后与原 PRG 逐字节一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/lift.mjs [--base <十六进制地址>] [--shift <十六进制字节数> [--split <十六进制地址>]] [--no-verify] [--why <地址>...] [--writes]
//       --base 把整个映像平移到别的加载地址再汇编（此时不与原映像比较，只汇编）；
//       --shift 把映像从分界地址起的部分整体后移指定字节（分界地址用 --split 指定，默认 $8000，即 $7B00-$7FFF 的常驻部分不动，中间填 $00），源码和二进制写在 build/ 里，用来配合运行测试找遗漏的地址常量；
//       --why 列出某些地址被谁引用；--writes 列出写入映像内地址的指令（用来发现自修改代码与别名地址）。
//       输入全部在 analysis/，加载与检查见 lib/image-model.mjs。立即数里拆开的地址写在 analysis/immediates.txt：
//       每行 “指令地址 lo|hi 目标地址[+偏移]”，该指令的立即数就是目标地址的低字节或高字节，汇编时写成 #<标签 或 #>标签。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { renderRoundtripConfig } from '../../../../packages/fcdev/src/disasm/index.mjs';
import { locateToolchain, runTool } from '../../../../packages/fcdev/src/toolchain/index.mjs';
import { BASE, PRG_SIZE, ROM_FILE, ROOT, collectTargets, dataElements, hex2, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode, formatInstruction } from './lib/opcode-modes.mjs';

const BUILD = join(ROOT, 'build');
const args = process.argv.slice(2);
const permuteIndex = args.indexOf('--permute');
const PERMUTE = permuteIndex >= 0 ? Number(args[permuteIndex + 1]) : null;
const baseIndex = args.indexOf('--base');
const LINK_BASE = baseIndex >= 0 ? Number.parseInt(args[baseIndex + 1], 16) : BASE;
const shiftIndex = args.indexOf('--shift');
const SHIFT = shiftIndex >= 0 ? Number.parseInt(args[shiftIndex + 1], 16) : 0;
const splitIndex = args.indexOf('--split');
const SPLIT_REQUEST = splitIndex >= 0 ? Number.parseInt(args[splitIndex + 1], 16) : 0x8000;
const SOURCE = PERMUTE !== null ? join(BUILD, 'fbasic-nes-permuted.s') : SHIFT > 0 ? join(BUILD, 'fbasic-nes-shift.s') : join(ROOT, 'reference', 'fbasic-nes.s');

let model;
try {
  model = loadImageModel();
} catch (error) {
  console.error(error.message);
  process.exit(1);
}
const { bytes, inImage, items, symbols, immediates, itemIndexAt } = model;
/** 后移的起点：不小于 --split 的第一个条目的起点（默认 $8000，也就是保持 $7B00-$7FFF 常驻部分不动） */
const RESIDENT_END = items.find((item) => item.start >= SPLIT_REQUEST)?.start ?? SPLIT_REQUEST;

// ---- 收集被引用的映像内地址 ----
const targets = collectTargets(model);
for (const address of immediates.keys()) {
  const item = items[itemIndexAt(address)];
  if (item.start !== address || item.type !== 'code' || addressingMode(item.opcode) !== 'imm') throw new Error(`immediates.txt：${hex4(address)} 不是立即数指令`);
}

const whyIndex = args.indexOf('--why');
if (whyIndex >= 0) for (const text of args.slice(whyIndex + 1)) console.log(`$${text}：${[...(targets.get(Number.parseInt(text, 16)) ?? [])].join('；')}`);

if (args.includes('--writes')) {
  const writers = ['sta', 'stx', 'sty', 'inc', 'dec', 'asl', 'lsr', 'rol', 'ror'];
  const found = new Map();
  for (const item of items) {
    if (item.type !== 'code' || !writers.includes(OPCODES[item.opcode][0])) continue;
    if (!['abs', 'abx', 'aby'].includes(addressingMode(item.opcode))) continue;
    const target = bytes.readUInt16LE(item.start - BASE + 1);
    if (!inImage(target)) continue;
    const owner = items[itemIndexAt(target)];
    const key = `${owner.type === 'data' ? owner.range.kind : owner.type} ${hex4(owner.start)}`;
    if (!found.has(key)) found.set(key, []);
    found.get(key).push(`${OPCODES[item.opcode][0]}@${hex4(item.start)}→${hex4(target)}`);
  }
  for (const [key, list] of found) console.log(`${key}：${list.slice(0, 4).join(' ')}${list.length > 4 ? ` 等 ${list.length} 处` : ''}`);
}

// ---- 标签：目标地址落在条目起点就直接标，落在字节类数据中间就拆开，落在其余条目中间就用等式 ----
const splitPoints = new Map();
const equates = [];
const labeled = new Set();
for (const address of [...targets.keys()].sort((left, right) => left - right)) {
  const index = itemIndexAt(address);
  const item = items[index];
  if (item.start === address) { labeled.add(address); continue; }
  const splittable = item.type === 'data' && ['fill', 'bytes', 'text'].includes(item.range.kind);
  const boundary = item.type === 'data' && !splittable && dataElements(item.range).some((element) => element.start === address);
  if (splittable || boundary) {
    if (!splitPoints.has(index)) splitPoints.set(index, new Set());
    splitPoints.get(index).add(address);
    labeled.add(address);
  } else {
    labeled.add(item.start);
    equates.push({ address, head: item.start });
  }
}

const labelName = (address) => symbols.get(address)?.name ?? `L${hex4(address)}`;
const externals = new Map();
/**
 * 映像外的地址写成符号名或数值。
 * @param {number} address 地址
 * @param {number} width 数值的十六进制位数
 * @returns {string} 表达式
 */
function externalName(address, width) {
  const symbol = symbols.get(address);
  if (symbol) { externals.set(address, symbol); return symbol.name; }
  return `$${address.toString(16).toUpperCase().padStart(width, '0')}`;
}
const addressExpression = (address, width = 4) => (inImage(address) ? labelName(address) : externalName(address, width));

// ---- 写源码 ----
const lines = [];
const out = (text = '') => lines.push(text);

/**
 * 写一条指令。
 * @param {object} item 指令条目
 * @returns {string} 一行 ca65 文字
 */
function instructionLine(item) {
  const offset = item.start - BASE;
  const mode = addressingMode(item.opcode);
  let operand = '';
  let forceAbsolute = false;
  if (mode === 'imm') {
    const mapping = immediates.get(item.start);
    if (mapping) {
      const base = labelName(mapping.target);
      operand = `${mapping.half === 'lo' ? '<' : '>'}${mapping.offset ? `(${base}+${mapping.offset})` : base}`;
    } else operand = `$${hex2(bytes[offset + 1])}`;
  } else if (mode === 'zp' || mode === 'zpx' || mode === 'zpy' || mode === 'izx' || mode === 'izy') {
    operand = addressExpression(bytes[offset + 1], 2);
  } else if (mode === 'abs' || mode === 'abx' || mode === 'aby' || mode === 'ind') {
    const value = bytes.readUInt16LE(offset + 1);
    operand = addressExpression(value);
    forceAbsolute = value < 0x100;
  } else if (mode === 'rel') {
    operand = addressExpression((item.start + 2 + bytes.readInt8(offset + 1)) & 0xffff);
  }
  return formatInstruction(item.opcode, operand, forceAbsolute);
}

/**
 * 写一串文字字节：可见的 ASCII 连成引号字符串，其余写数值；遇到 0 结束一行。
 * @param {number[]} values 字节
 * @returns {string[]} 若干行 .byte
 */
function textLines(values) {
  const result = [];
  let pieces = [];
  let quoted = '';
  const flushQuoted = () => { if (quoted !== '') { pieces.push(`"${quoted}"`); quoted = ''; } };
  const flushLine = () => { flushQuoted(); if (pieces.length > 0) { result.push(`        .byte   ${pieces.join(', ')}`); pieces = []; } };
  for (const value of values) {
    if (value >= 0x20 && value < 0x7f && value !== 0x22) {
      quoted += String.fromCharCode(value);
      if (quoted.length >= 40) flushQuoted();
    } else {
      flushQuoted();
      pieces.push(`$${hex2(value)}`);
      if (value === 0 || pieces.length >= 12) flushLine();
    }
  }
  flushLine();
  return result;
}

/**
 * 写一段数据（已经按标签拆好的一块）。
 * @param {object} range 布局区段
 * @param {number} start 这一块的起点
 * @param {number} end 这一块的终点（含）
 */
function emitData(range, start, end) {
  const slice = (from, to) => [...bytes.subarray(from - BASE, to - BASE + 1)];
  if (range.kind === 'fill') {
    out(`        .res    ${end - start + 1}, $${hex2(bytes[start - BASE])}`);
  } else if (range.kind === 'bytes') {
    const values = slice(start, end);
    for (let at = 0; at < values.length; at += 16) out(`        .byte   ${values.slice(at, at + 16).map((value) => `$${hex2(value)}`).join(',')}`);
  } else if (range.kind === 'text') {
    for (const line of textLines(slice(start, end))) out(line);
  } else {
    const elements = dataElements(range).filter((element) => element.start >= start && element.start <= end);
    let pending = [];
    let pendingKind = null;
    const flush = () => {
      if (pending.length === 0) return;
      const directive = pendingKind === 'b' ? '.byte' : pendingKind === 'a' ? '.addr' : '.word';
      for (let at = 0; at < pending.length; at += 8) out(`        ${directive.padEnd(8)}${pending.slice(at, at + 8).join(',')}`);
      pending = [];
    };
    for (const element of elements) {
      if (element.kind !== pendingKind) { flush(); pendingKind = element.kind; }
      if (element.kind === 'b') pending.push(`$${hex2(bytes[element.start - BASE])}`);
      else {
        const value = bytes.readUInt16LE(element.start - BASE);
        pending.push(element.kind === 'a' ? addressExpression(value) : `$${hex4(value)}`);
      }
    }
    flush();
  }
}

let highStarted = false;
/** 可以切开重排的位置（lines 的下标）：前一个条目是无条件跳转、返回的代码，或者是数据 */
const cutPoints = [];
items.forEach((item, index) => {
  if (index > 0) {
    const previous = items[index - 1];
    const ends = previous.type === 'data' || (previous.type === 'code' && ['jump', 'jump-indirect', 'return', 'stop'].includes(OPCODES[previous.opcode][2]));
    if (ends) cutPoints.push({ index, line: lines.length });
  }
  if (SHIFT > 0 && !highStarted && item.start >= RESIDENT_END) {
    highStarted = true;
    out('        .segment "HIGH"');
  }
  if (item.type === 'data') {
    const cuts = [...(splitPoints.get(index) ?? [])].sort((left, right) => left - right);
    const edges = [item.start, ...cuts, item.start + item.size];
    for (let part = 0; part + 1 < edges.length; part += 1) {
      if (labeled.has(edges[part])) out(`${labelName(edges[part])}:`);
      emitData(item.range, edges[part], edges[part + 1] - 1);
    }
    return;
  }
  const label = labeled.has(item.start) ? `${labelName(item.start)}:` : '';
  const text = item.type === 'skip' ? `.byte   $${hex2(bytes[item.start - BASE])}` : instructionLine(item);
  out(label === '' ? `        ${text}` : `${label.padEnd(8)}${text}`);
});

// ---- 文件头、外部符号与等式 ----
const header = [
  '; ------------------------------------------------------------------------',
  '; 名称：fbasic-nes.s',
  '; 说明：Family BASIC v3.0（NES，mapper 0）PRG 的可重定位符号源码（原版，未改动）：汇编后与原 PRG 逐字节一致，所有地址都用标签。',
  '; 作者：sungbin',
  '; 邮箱：sungbin@3578.cn',
  '; 日期：2026-11-16',
  '; 备注：由 tools/lift.mjs 根据 analysis/ 里的分析结果生成，不要手改；要改动请改分析文件后重新生成，原生版的源码在 exe/。',
  '; ------------------------------------------------------------------------',
  '        .setcpu "6502"',
  '',
];
const prelude = [];
for (const [address, symbol] of [...externals].sort((left, right) => left[0] - right[0])) {
  prelude.push(`${symbol.name.padEnd(24)}:= $${address.toString(16).toUpperCase().padStart(address < 0x100 ? 2 : 4, '0')}${symbol.note ? `  ; ${symbol.note}` : ''}`);
}
if (equates.length > 0) {
  prelude.push('', '; 指向指令或数据中间的地址');
  for (const equate of equates.sort((left, right) => left.address - right.address)) {
    prelude.push(`${labelName(equate.address).padEnd(24)}:= ${labelName(equate.head)}+${equate.address - equate.head}`);
  }
}
/**
 * 把源码按可切开的位置切成块，除第一块（复位入口）和最后一块（向量）外用种子打乱顺序。
 * @param {number} seed 种子
 * @returns {string[]} 重排后的源码行
 */
function permutedLines(seed) {
  // 相对分支的范围有限，被分支跨过的位置不能切
  const blocked = new Set();
  items.forEach((item, index) => {
    if (item.type !== 'code' || addressingMode(item.opcode) !== 'rel') return;
    const target = itemIndexAt((item.start + 2 + bytes.readInt8(item.start - BASE + 1)) & 0xffff);
    for (let position = Math.min(index, target) + 1; position <= Math.max(index, target); position += 1) blocked.add(position);
  });
  const starts = [0, ...cutPoints.filter((cut) => !blocked.has(cut.index)).map((cut) => cut.line)];
  const chunks = starts.map((start, index) => lines.slice(start, starts[index + 1] ?? lines.length));
  const middle = chunks.slice(1, -1);
  let state = seed >>> 0;
  const random = () => { state = (state + 0x6d2b79f5) >>> 0; let value = state; value = Math.imul(value ^ (value >>> 15), value | 1); value ^= value + Math.imul(value ^ (value >>> 7), value | 61); return ((value ^ (value >>> 14)) >>> 0) / 4294967296; };
  for (let index = middle.length - 1; index > 0; index -= 1) { const other = Math.floor(random() * (index + 1)); [middle[index], middle[other]] = [middle[other], middle[index]]; }
  console.log(`重排 ${middle.length} 块（共 ${chunks.length} 块，首尾两块不动）`);
  return [...chunks[0], ...middle.flat(), ...chunks.at(-1)];
}
const source = `${[...header, ...prelude, '', ...(PERMUTE !== null ? permutedLines(PERMUTE) : lines)].join('\n')}\n`;
mkdirSync(dirname(SOURCE), { recursive: true });
writeFileSync(SOURCE, source);
console.log(`写入 ${SOURCE}：${items.length} 个条目，${targets.size} 个被引用的地址，${equates.length} 个中间地址等式`);

// ---- 汇编并核对 ----
/**
 * 后移高半部分用的链接配置：常驻部分 $7B00-$7FFF，中间填充，高半部分从 $8000+偏移 起。
 * @returns {string} ld65 配置
 */
function shiftedConfig() {
  const hex = (value) => `$${value.toString(16).toUpperCase()}`;
  return `MEMORY {
  LOW: file = %O, start = ${hex(BASE)}, size = ${hex(RESIDENT_END - BASE)};
  GAP: file = %O, start = ${hex(RESIDENT_END)}, size = ${hex(SHIFT)}, fill = yes;
  HIGH: file = %O, start = ${hex(RESIDENT_END + SHIFT)}, size = ${hex(bytes.length - (RESIDENT_END - BASE))};
}
SEGMENTS { CODE: load = LOW, type = ro; HIGH: load = HIGH, type = ro; }
`;
}
if (!args.includes('--no-verify')) {
  mkdirSync(BUILD, { recursive: true });
  const toolchain = locateToolchain({});
  const suffix = PERMUTE !== null ? '-permuted' : SHIFT > 0 ? '-shift' : '';
  const objectFile = join(BUILD, `fbasic-nes${suffix}.o`);
  const configFile = join(BUILD, `fbasic-nes${suffix}.cfg`);
  const binaryFile = join(BUILD, `fbasic-nes${suffix}.bin`);
  writeFileSync(configFile, SHIFT > 0 ? shiftedConfig() : renderRoundtripConfig(LINK_BASE, bytes.length));
  for (const [tool, toolArgs] of [['ca65', [SOURCE, '-o', objectFile]], ['ld65', ['-C', configFile, '-o', binaryFile, objectFile]]]) {
    const result = await runTool(toolchain, tool, toolArgs);
    if (result.code !== 0) { console.error(`${tool} 失败：\n${result.stdout}${result.stderr}`.trim()); process.exit(1); }
  }
  const built = readFileSync(binaryFile);
  if (PERMUTE !== null) {
    const rom = readFileSync(ROM_FILE);
    if (built.length !== PRG_SIZE) { console.error(`重排后的 PRG 大小不对：${built.length}`); process.exit(1); }
    const nesFile = join(BUILD, 'fbasic-permuted.nes');
    writeFileSync(nesFile, Buffer.concat([rom.subarray(0, 16), built, rom.subarray(16 + PRG_SIZE)]));
    console.log(`已生成重排的 ROM：${nesFile}`);
    process.exit(0);
  }
  if (LINK_BASE !== BASE || SHIFT > 0) {
    console.log(`已汇编：${built.length} 字节（平移或后移时不与原映像比较），输出 ${binaryFile}`);
  } else {
    let first = -1;
    for (let offset = 0; offset < Math.max(built.length, bytes.length); offset += 1) if (built[offset] !== bytes[offset]) { first = offset; break; }
    if (first >= 0) { console.error(`回读校验不一致：偏移 ${first}（$${hex4(BASE + first)}）起不同`); process.exit(1); }
    console.log(`回读校验通过：${built.length} 字节与原映像逐字节一致`);
  }
}
