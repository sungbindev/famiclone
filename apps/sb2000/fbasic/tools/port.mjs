// ------------------------------------------------------------------------
// 名称：port.mjs
// 说明：把原版源码按 analysis/banks.txt 的分配拆成常驻页与各库页，生成 exe/src/core.s（解释器）、跳板与 exe/src/layout.inc（页号常量）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/port.mjs；不手改 exe/src/core.s。分页方案见 docs/移植对照表.md 第 3.4 节。
//       转向非常驻块的 jsr、jmp 与地址表里的字都改成指向跳板（标签后缀 _far），跳板在常驻页里：jsr far_enter，后面跟目标地址与库页号；
//       far_enter 与 platform.s 里写的 L 标签引用由 platform.s 提供，本工具只负责把 platform.s 用到的 L 标签导出，并为它写了 _far 的标签生成跳板。
//       所有 rti 改成 rts（NMI 例程改由钩子 5 调用）；analysis/port-remove.txt 里列出的地址范围整段删除（每行：起-止 说明）。
// ------------------------------------------------------------------------
import { existsSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, ROOT, hex4 } from './lib/image-model.mjs';
import { buildChunks } from './lib/chunks.mjs';
import { createEmitter } from './lib/emit.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';
import { readRangeList } from './lib/analysis-files.mjs';

const SRC = join(ROOT, 'exe', 'src');
const BANK_SIZE = 4096;
const RESIDENT_DATA_LIMIT = 600;
const NMI_ENTRY = 0x8971;
// 内核的 NMI 钩子 5 经零页 $1E/$1F 跳转，原版用到这两个单元，改用原版没用过的 $38/$39（成对使用的指针保持相邻）
const ZERO_PAGE_MAP = new Map([[0x1e, 0x38], [0x1f, 0x39]]);
// 原版把 $0600-$06FF 当自己的数组用（会清整页），这一页在学习机里放着内核的影子与窗口页号；改到常驻页里的 fb_page6
const mapAddress = (address) => {
  if (address >= 0x0600 && address <= 0x06ff) return address === 0x0600 ? 'fb_page6' : `fb_page6+${address - 0x0600}`;
  if (address >= TEXT_AREA_START && address <= TEXT_AREA_END) return `$${(address + TEXT_AREA_SHIFT).toString(16).toUpperCase()}`;
  return ZERO_PAGE_MAP.get(address);
};
// 程序文本区：原版在 $6000-$6FFF（4K，$6C00-$6FFF 还被 BG 画面的备份占用），移到窗口 3（$B000-$BFFF）；
// 这些立即数是文本区地址的高字节（逐处核对过，见 docs/移植对照表.md），要同步加偏移，其余像 $64（十进制 100）、低字节的 $60 都不是地址
const TEXT_AREA_START = 0x6000;
const TEXT_AREA_END = 0x7fff;
const TEXT_AREA_SHIFT = 0x5000;
const TEXT_HIGH_BYTE_SITES = new Set([0x8148, 0x84d8, 0x86a3, 0x97d7, 0xadc8, 0xb1bd, 0xb1ca, 0xb20b]);

const { model, chunks, chunkAt } = buildChunks();
const { items, bytes } = model;

// ---- 代码块的位置 ----
const location = new Map();
for (const line of readFileSync(join(ROOT, 'analysis', 'banks.txt'), 'utf8').split(/\r?\n/)) {
  const text = line.replace(/#.*$/, '').trim();
  if (text === '') continue;
  const [start, place] = text.split(/\s+/);
  location.set(chunkAt(Number.parseInt(start, 16)), place);
}
const codeBanks = Math.max(0, ...[...location.values()].filter((place) => place !== 'R').map(Number));

// ---- 数据块的位置：大的放数据库页（按地址顺序装进 4K 页），小的看谁在用 ----
const dataRefs = new Map();
const tablePointed = new Set();
const note = (target, place) => { if (!dataRefs.has(target)) dataRefs.set(target, new Set()); dataRefs.get(target).add(place); };
items.forEach((item) => {
  if (item.type !== 'code') return;
  const mode = addressingMode(item.opcode);
  const place = location.get(chunkAt(item.start));
  if (['abs', 'abx', 'aby'].includes(mode) && !['jsr', 'jmp'].includes(OPCODES[item.opcode][0])) {
    const target = bytes.readUInt16LE(item.start - BASE + 1);
    if (target >= BASE) note(chunkAt(target), place);
  } else if (mode === 'imm' && model.immediates.has(item.start)) note(chunkAt(model.immediates.get(item.start).target), place);
});
for (const item of items) {
  if (item.type !== 'data' || item.range.kind !== 'addrs') continue;
  for (let address = item.start; address < item.start + item.size; address += 2) {
    const target = bytes.readUInt16LE(address - BASE);
    if (target >= BASE) tablePointed.add(chunkAt(target));
  }
}
const bankUsed = new Map();
for (const chunk of chunks.filter((entry) => entry.kind === 'code')) {
  const place = location.get(chunk.id);
  if (place !== 'R') bankUsed.set(place, (bankUsed.get(place) ?? 0) + chunk.size);
}
let dataBank = codeBanks;
let dataBankUsed = BANK_SIZE;
const dataBanks = [];
for (const chunk of chunks.filter((entry) => entry.kind === 'data')) {
  if (chunk.size > RESIDENT_DATA_LIMIT) {
    if (dataBankUsed + chunk.size > BANK_SIZE) { dataBank += 1; dataBankUsed = 0; dataBanks.push({ bank: dataBank, chunks: [] }); }
    dataBankUsed += chunk.size;
    dataBanks.at(-1).chunks.push(chunk);
    location.set(chunk.id, String(dataBank));
    continue;
  }
  const users = dataRefs.get(chunk.id) ?? new Set();
  const only = users.size === 1 ? [...users][0] : null;
  if (only !== null && only !== 'R' && !tablePointed.has(chunk.id) && (bankUsed.get(only) ?? 0) + chunk.size <= BANK_SIZE) {
    location.set(chunk.id, only);
    bankUsed.set(only, (bankUsed.get(only) ?? 0) + chunk.size);
  } else location.set(chunk.id, 'R');
}
const imageBank = dataBank + 1;
const totalBanks = imageBank;

// ---- 删除范围 ----
const removeFile = join(ROOT, 'analysis', 'port-remove.txt');
const removed = existsSync(removeFile) ? readRangeList(removeFile) : [];
const isRemoved = (address) => removed.some((range) => address >= range.start && address <= range.end);

// ---- platform 源码里用到的 L 标签 ----
const platformFiles = readdirSync(SRC).filter((name) => name.endsWith('.s') && name !== 'core.s');
const platformText = platformFiles.map((name) => readFileSync(join(SRC, name), 'utf8')).join('\n');
const platformLabels = new Set([...platformText.matchAll(/\bL([0-9A-F]{4})(_far)?\b/g)].map((match) => `${match[1]}${match[2] ?? ''}`));

// ---- 跳板 ----
const stubs = new Map();
const placeOfAddress = (address) => location.get(chunkAt(address));
const stubFor = (target) => {
  const place = placeOfAddress(target);
  if (place === 'R') return undefined;
  stubs.set(target, place);
  return `L${hex4(target)}_far`;
};
for (const name of platformLabels) if (name.endsWith('_far')) stubFor(Number.parseInt(name.slice(0, 4), 16));
const farAliases = [...platformLabels].filter((name) => name.endsWith('_far') && !stubs.has(Number.parseInt(name.slice(0, 4), 16))).map((name) => `L${name}`);

const emitter = createEmitter(model, {
  mapAddress,
  codeOperand(item, target) {
    const from = placeOfAddress(item.start);
    const to = placeOfAddress(target);
    if (to === 'R' || to === from) return undefined;
    return stubFor(target);
  },
  tableWord(address, value) {
    if (value < BASE || value >= BASE + bytes.length) return undefined;
    if (model.items[model.itemIndexAt(value)].type !== 'code') return undefined;
    return stubFor(value);
  },
  instruction(item) {
    if (isRemoved(item.start)) return '';
    const mnemonic = OPCODES[item.opcode][0];
    // NMI 例程原来入口 pha、出口 pla + rti；现在由钩子调用、以 rts 返回，寄存器由内核保存，所以这一对成对去掉
    // （经跳板调用的例程不能靠栈里留着调用前压的值）
    if (item.start === NMI_ENTRY) return '';
    if (mnemonic === 'pla' && OPCODES[items[model.itemIndexAt(item.start) + 1].opcode]?.[0] === 'rti') return '';
    if (mnemonic === 'rti') return 'rts';
    if (TEXT_HIGH_BYTE_SITES.has(item.start)) return `${mnemonic.padEnd(8)}#$${(bytes[item.start - BASE + 1] + (TEXT_AREA_SHIFT >> 8)).toString(16).toUpperCase().padStart(2, '0')}`;
    return undefined;
  },
});

// ---- 按位置收集源码 ----
const segments = new Map();
const segmentOf = (chunk) => {
  const place = location.get(chunk.id);
  if (place === 'R') return chunk.kind === 'code' ? 'CODE' : 'RODATA';
  return `BANK${place}`;
};
for (const chunk of chunks) {
  const name = segmentOf(chunk);
  if (!segments.has(name)) segments.set(name, []);
  const out = segments.get(name);
  out.push(`; ---- $${hex4(chunk.start)}-$${hex4(chunk.end)}（${chunk.kind === 'code' ? '代码' : '数据'} ${chunk.size} 字节）`);
  for (let index = chunk.first; index <= chunk.last; index += 1) out.push(...emitter.emitItem(index));
}

// ---- 汇总、导出与跳板 ----
const sizeOf = (name) => chunks.filter((chunk) => segmentOf(chunk) === name).reduce((sum, chunk) => sum + chunk.size, 0);
const stubNames = [...platformLabels].filter((name) => name.endsWith('_far') && stubs.has(Number.parseInt(name.slice(0, 4), 16))).map((name) => `L${name}`);
const exportsList = [...[...platformLabels].filter((name) => !name.endsWith('_far')).map((name) => `L${name}`).filter((name) => emitter.labeled.has(Number.parseInt(name.slice(1), 16))), ...stubNames, ...farAliases];
const stubLines = [];
for (const [target, place] of [...stubs].sort((left, right) => left[0] - right[0])) {
  stubLines.push(`L${hex4(target)}_far:`, '        jsr     far_enter', `        .word   L${hex4(target)}`, `        .byte   ${place}`);
}
const header = [
  '; ------------------------------------------------------------------------',
  '; 名称：core.s',
  '; 说明：Family BASIC v3 解释器的 SB2000 移植版（由 tools/port.mjs 生成，不要手改）：按 analysis/banks.txt 分成常驻页与各库页，转向库页的 jsr、jmp 与地址表都经跳板。',
  '; 作者：sungbin',
  '; 邮箱：sungbin@3578.cn',
  '; 日期：2026-11-16',
  '; 备注：平台相关的部分（入口、NMI 钩子、跳板进入例程 far_enter、入口门、键盘与画面等）在 platform.s 等手写文件里；要改动请改分析文件或生成器后重新生成。',
  '; ------------------------------------------------------------------------',
  '        .setcpu "6502"',
  '        .import far_enter, fb_page6',
  ...(exportsList.length > 0 ? [`        .export ${exportsList.join(', ')}`] : []),
  '',
];
const body = [];
for (const name of ['CODE', 'RODATA', ...[...segments.keys()].filter((key) => key.startsWith('BANK')).sort((left, right) => Number(left.slice(4)) - Number(right.slice(4)))]) {
  if (!segments.has(name)) continue;
  body.push(`        .segment "${name}"`, ...segments.get(name), '');
  if (name === 'CODE') body.push('; ---- 跳板', ...stubLines, '');
}
body.push(`        .segment "BANK${imageBank}"`, '; ---- 现场缓冲：7 页（零页与 $0200-$07FF），由 platform.s 里的现场换入换出使用', '        .res    1792, $00', '');
if (farAliases.length > 0) body.push('; ---- 平台用的别名：目标在常驻页里，不需要跳板', ...farAliases.map((name) => `${name} = ${name.replace('_far', '')}`), '');
writeFileSync(join(SRC, 'core.s'), `${[...header, ...emitter.preludeLines(), '', ...body].join('\n')}\n`);

const layout = [
  '; 由 tools/port.mjs 生成：页号常量（库页号从 1 起，对应 start.s 里的 bank_select）',
  `CODE_BANKS       = ${codeBanks}`,
  `TOTAL_BANKS      = ${totalBanks}`,
  `BANK_IMAGE       = ${imageBank}                  ; 两份现场缓冲`,
  ...dataBanks.map((entry) => `BANK_DATA_${entry.bank}     = ${entry.bank}                  ; ${entry.chunks.map((chunk) => `$${hex4(chunk.start)}-$${hex4(chunk.end)}`).join('、')}`),
  '',
];
writeFileSync(join(SRC, 'layout.inc'), layout.join('\n'));

const residentCode = sizeOf('CODE');
const residentData = sizeOf('RODATA');
console.log(`常驻：代码 ${residentCode} 字节，数据 ${residentData} 字节，跳板 ${stubs.size} 个（${stubs.size * 6} 字节）；库页 ${totalBanks} 个（代码 ${codeBanks}，数据 ${totalBanks - 1 - codeBanks}，现场缓冲 1）`);
for (const [name, list] of [...segments].filter(([key]) => key.startsWith('BANK'))) console.log(`${name}：${sizeOf(name)} 字节`);
console.log(`写入 exe/src/core.s 与 exe/src/layout.inc；fcdev.json 的 banks 需要是 ${totalBanks}`);
