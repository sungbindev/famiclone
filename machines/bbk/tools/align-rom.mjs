// ------------------------------------------------------------------------
// 名称：align-rom.mjs
// 说明：把源码汇编结果与 ROM 最后一页按“指令块”对齐，报告源码独有的区段与 ROM 独有的区段。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/align-rom.mjs [源文件] [--rom|二进制文件] [块长]
//       源码里的指令按每块 N 条（缺省 6）切开，带绝对地址的指令把两字节地址当通配符，
//       在 ROM 最后一页里查找；找到的块覆盖 ROM 字节，没找到的块是源码里有而 ROM 里没有的。
//       ROM 里没有被覆盖、又不是 $FF/$00 填充的区段，是 ROM 里有而源码里没有的，
//       其中包含字库与数据表，需要结合反汇编判断是不是代码。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { Gasm } from './gasm.mjs';
import { INCLUDE_DIR, ORIGINAL_SOURCE_DIR, ROM_BIOS10 } from './config.mjs';

const sourceName = process.argv[2] ?? 'Bios.asm';
const binaryArg = process.argv[3] ?? '--rom';
const groupSize = Number(process.argv[4] ?? 6);
const BANK_SIZE = 0x4000;
const BASE = 0xc000;

const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(join(ORIGINAL_SOURCE_DIR, sourceName));
const file = readFileSync(binaryArg === '--rom' ? ROM_BIOS10 : binaryArg);
const bank = file.subarray(file.length - BANK_SIZE);

/** 在 ROM 页里查找带通配符（-1）的模式，返回所有命中偏移 */
function findAll(pattern) {
  const hits = [];
  for (let start = 0; start + pattern.length <= bank.length; start += 1) {
    let ok = true;
    for (let i = 0; i < pattern.length; i += 1) {
      if (pattern[i] !== -1 && bank[start + i] !== pattern[i]) {
        ok = false;
        break;
      }
    }
    if (ok) hits.push(start);
  }
  return hits;
}

/** 指令字节的模式：三字节指令的地址部分为通配符 */
const maskOf = (bytes) => (bytes.length === 3 ? [bytes[0], -1, -1] : bytes);

const covered = new Uint8Array(BANK_SIZE);
const matchedChunks = [];
const missingChunks = [];
let run = [];

/** 把一段连续指令切块并查找 */
function flush() {
  for (let start = 0; start < run.length; start += groupSize) {
    const chunk = run.slice(start, start + groupSize);
    if (chunk.length < 3) continue;
    const pattern = chunk.flatMap((entry) => maskOf(entry.bytes));
    const hits = findAll(pattern);
    const first = chunk[0].address;
    const length = chunk.reduce((sum, entry) => sum + entry.bytes.length, 0);
    if (hits.length > 0) {
      // 多处命中时取与源码地址最接近的一处
      const best = hits.reduce((a, b) => (Math.abs(a - (first - BASE)) <= Math.abs(b - (first - BASE)) ? a : b));
      for (let i = 0; i < length; i += 1) covered[best + i] = 1;
      matchedChunks.push({ first, length, offset: best, ambiguous: hits.length > 1 });
    } else {
      missingChunks.push({ first, length });
    }
  }
  run = [];
}

for (const entry of result.listing) {
  if (entry.kind === 'ins' && entry.address >= BASE) run.push(entry);
  else flush();
}
flush();

const sum = (items) => items.reduce((total, item) => total + item.length, 0);
const hex = (value) => `$${value.toString(16).padStart(4, '0')}`;
const merge = (items) => {
  const regions = [];
  for (const item of items) {
    const last = regions[regions.length - 1];
    if (last && item.first <= last.end + 1) last.end = Math.max(last.end, item.first + item.length - 1);
    else regions.push({ start: item.first, end: item.first + item.length - 1 });
  }
  return regions;
};

console.log(`源码指令 ${sum(matchedChunks) + sum(missingChunks)} 字节：在 ROM 里找到 ${sum(matchedChunks)} 字节（${Math.round((100 * sum(matchedChunks)) / (sum(matchedChunks) + sum(missingChunks)))}%），没找到 ${sum(missingChunks)} 字节`);
const deltas = new Map();
for (const chunk of matchedChunks) {
  const delta = chunk.offset - (chunk.first - BASE);
  deltas.set(delta, (deltas.get(delta) ?? 0) + chunk.length);
}
console.log('源码地址到 ROM 地址的偏移（字节数最多的前 8 个）：');
for (const [delta, bytes] of [...deltas.entries()].sort((a, b) => b[1] - a[1]).slice(0, 8)) console.log(`  ${delta >= 0 ? '+' : '-'}${Math.abs(delta)}：${bytes} 字节`);

const sourceOnly = merge(missingChunks);
console.log(`源码独有的区段 ${sourceOnly.length} 个：`);
for (const region of sourceOnly.slice(0, 60)) console.log(`  ${hex(region.start)}-${hex(region.end)}（${region.end - region.start + 1} 字节）`);

const romOnly = [];
for (let i = 0; i < BANK_SIZE; ) {
  if (covered[i]) {
    i += 1;
    continue;
  }
  let j = i;
  while (j < BANK_SIZE && !covered[j]) j += 1;
  const slice = bank.subarray(i, j);
  const filler = slice.every((byte) => byte === 0xff || byte === 0x00);
  if (j - i >= 16 && !filler) romOnly.push({ start: BASE + i, end: BASE + j - 1 });
  i = j;
}
console.log(`ROM 独有或未覆盖的区段（不含 $FF/$00 填充）${romOnly.length} 个，共 ${romOnly.reduce((total, region) => total + region.end - region.start + 1, 0)} 字节：`);
for (const region of romOnly.slice(0, 60)) console.log(`  ${hex(region.start)}-${hex(region.end)}（${region.end - region.start + 1} 字节）`);
