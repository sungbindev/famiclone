// ------------------------------------------------------------------------
// 名称：lift.mjs
// 说明：把 Family BASIC v3 的 PRG 还原成可重定位的 ca65 源码 reference/fbasic-nes.s：代码、数据、地址表都用标签，汇编后与原 PRG 逐字节一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/lift.mjs [--base <十六进制地址>] [--permute <种子>] [--no-verify] [--why <地址>...] [--writes]
//       --base 把整个 PRG 平移到别的加载地址再汇编（此时不与原 PRG 比较，只汇编）；
//       --permute 把块（lib/chunks.mjs 的切法）按种子打乱顺序后重新汇编，生成 build/fbasic-permuted.nes，用来在 Ember 里找遗漏的地址常量；
//       --why 列出某些地址被谁引用；--writes 列出写入 PRG 内地址的指令（用来发现自修改代码与别名地址）。
//       输入全部在 analysis/，加载与检查见 lib/image-model.mjs，写源码见 lib/emit.mjs。立即数里拆开的地址写在 analysis/immediates.txt：
//       每行 “指令地址 lo|hi 目标地址[+偏移]”，该指令的立即数就是目标地址的低字节或高字节，汇编时写成 #<标签 或 #>标签。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { locateToolchain, runTool } from '../../../../packages/fcdev/src/toolchain/index.mjs';
import { BASE, PRG_SIZE, ROM_FILE, ROOT, hex4, loadImageModel } from './lib/image-model.mjs';
import { createEmitter } from './lib/emit.mjs';
import { OPCODES } from './lib/opcodes.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

/**
 * 回读用的链接配置：一个从起始地址开始、大小恰好等于映像的内存区与一个 CODE 段。
 * @param {number} base 起始地址
 * @param {number} size 映像大小
 * @returns {string} ld65 配置内容
 */
function renderRoundtripConfig(base, size) {
  return `MEMORY { ROM: file = %O, start = $${base.toString(16).toUpperCase()}, size = $${size.toString(16).toUpperCase()}; }
SEGMENTS { CODE: load = ROM, type = ro; }
`;
}

const BUILD = join(ROOT, 'build');
const args = process.argv.slice(2);
const baseIndex = args.indexOf('--base');
const LINK_BASE = baseIndex >= 0 ? Number.parseInt(args[baseIndex + 1], 16) : BASE;
const permuteIndex = args.indexOf('--permute');
const PERMUTE = permuteIndex >= 0 ? Number(args[permuteIndex + 1]) : null;
const SOURCE = PERMUTE !== null ? join(BUILD, 'fbasic-nes-permuted.s') : join(ROOT, 'reference', 'fbasic-nes.s');

let model;
try {
  model = loadImageModel();
} catch (error) {
  console.error(error.message);
  process.exit(1);
}
const { bytes, inImage, items, immediates, itemIndexAt } = model;
const emitter = createEmitter(model);
const { targets } = emitter;
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

// ---- 写源码；同时记下可以切开重排的位置（lines 的下标）----
const lines = [];
const cutPoints = [];
items.forEach((item, index) => {
  if (index > 0) {
    const previous = items[index - 1];
    const ends = previous.type === 'data' || (previous.type === 'code' && ['jump', 'jump-indirect', 'return', 'stop'].includes(OPCODES[previous.opcode][2]));
    if (ends) cutPoints.push({ index, line: lines.length });
  }
  lines.push(...emitter.emitItem(index));
});

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

/**
 * 把源码按可切开的位置切成块，除第一块（复位入口）和最后一块（向量）外用种子打乱顺序。
 * 被相对分支跨过的位置不能切。
 * @param {number} seed 种子
 * @returns {string[]} 重排后的源码行
 */
function permutedLines(seed) {
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
const source = `${[...header, ...emitter.preludeLines(), '', ...(PERMUTE !== null ? permutedLines(PERMUTE) : lines)].join('\n')}\n`;
mkdirSync(dirname(SOURCE), { recursive: true });
writeFileSync(SOURCE, source);
console.log(`写入 ${SOURCE}：${items.length} 个条目，${targets.size} 个被引用的地址，${emitter.equates.length} 个中间地址等式`);

// ---- 汇编并核对 ----
if (!args.includes('--no-verify')) {
  mkdirSync(BUILD, { recursive: true });
  const toolchain = locateToolchain({});
  const suffix = PERMUTE !== null ? '-permuted' : '';
  const objectFile = join(BUILD, `fbasic-nes${suffix}.o`);
  const configFile = join(BUILD, `fbasic-nes${suffix}.cfg`);
  const binaryFile = join(BUILD, `fbasic-nes${suffix}.bin`);
  writeFileSync(configFile, renderRoundtripConfig(LINK_BASE, bytes.length));
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
  if (LINK_BASE !== BASE) {
    console.log(`已汇编：${built.length} 字节（平移时不与原 PRG 比较），输出 ${binaryFile}`);
  } else {
    let first = -1;
    for (let offset = 0; offset < Math.max(built.length, bytes.length); offset += 1) if (built[offset] !== bytes[offset]) { first = offset; break; }
    if (first >= 0) { console.error(`回读校验不一致：偏移 ${first}（$${hex4(BASE + first)}）起不同`); process.exit(1); }
    console.log(`回读校验通过：${built.length} 字节与原 PRG 逐字节一致`);
  }
}
