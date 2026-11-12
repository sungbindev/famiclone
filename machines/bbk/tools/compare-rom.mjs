// ------------------------------------------------------------------------
// 名称：compare-rom.mjs
// 说明：把汇编结果与原厂二进制逐字节比对，报告相同字节数、差异区段与首批差异。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/compare-rom.mjs <源文件> <二进制文件> <二进制偏移> [起始地址]
//       二进制偏移为“汇编地址 起始地址 对应的文件偏移”，起始地址缺省为汇编结果的最小地址。
//       ROM 用 --rom 简写：node tools/compare-rom.mjs Bios.asm --rom（取 BIOS 1.0 ROM 的最后 16K）。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { Gasm } from './gasm.mjs';
import { INCLUDE_DIR, INES_HEADER_SIZE, ORIGINAL_SOURCE_DIR, ROM_BIOS10 } from './config.mjs';

const [sourceName, binaryArg, offsetArg, startArg] = process.argv.slice(2);
if (!sourceName || !binaryArg) {
  console.error('用法：node tools/compare-rom.mjs <源文件> <二进制文件|--rom> [偏移] [起始地址]');
  process.exit(2);
}

const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(join(ORIGINAL_SOURCE_DIR, sourceName));
const addresses = [...result.memory.keys()].sort((a, b) => a - b);
const first = startArg ? Number.parseInt(startArg.replace('$', ''), 16) : addresses[0];

let binary;
let baseOffset;
if (binaryArg === '--rom') {
  binary = readFileSync(ROM_BIOS10);
  baseOffset = binary.length - 0x4000;
} else {
  binary = readFileSync(binaryArg);
  baseOffset = Number(offsetArg ?? 0);
}

const same = [];
const different = [];
for (const address of addresses) {
  const offset = baseOffset + (address - first);
  if (offset < 0 || offset >= binary.length) continue;
  (binary[offset] === result.memory.get(address) ? same : different).push(address);
}

// 把差异地址合并成连续区段
const regions = [];
for (const address of different) {
  const last = regions[regions.length - 1];
  if (last && address - last.end <= 1) last.end = address;
  else regions.push({ start: address, end: address });
}

const hex = (value) => `$${value.toString(16).padStart(4, '0')}`;
console.log(`汇编 ${addresses.length} 字节，相同 ${same.length}，不同 ${different.length}`);
console.log(`差异区段 ${regions.length} 个：`);
for (const region of regions.slice(0, 40)) {
  const length = region.end - region.start + 1;
  const src = [...Array(Math.min(length, 8)).keys()].map((i) => result.memory.get(region.start + i).toString(16).padStart(2, '0')).join(' ');
  const rom = [...Array(Math.min(length, 8)).keys()]
    .map((i) => binary[baseOffset + region.start + i - first].toString(16).padStart(2, '0'))
    .join(' ');
  console.log(`  ${hex(region.start)}-${hex(region.end)} (${length} 字节)  汇编 ${src}  二进制 ${rom}`);
}
