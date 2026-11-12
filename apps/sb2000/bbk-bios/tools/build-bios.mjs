// ------------------------------------------------------------------------
// 名称：build-bios.mjs
// 说明：把 src/Bios.asm 汇编成 bios.bbk：16 字节文件头加 8 个 16K 页，供 START.EXE 加载。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/build-bios.mjs [输出文件]，缺省输出 build/bin/bios.bbk。
//       文件头：0-3 魔数 "BBKB"，4 格式版本（1），5 页数（8），6-7 入口地址（复位入口 Begin，小端），8-15 保留为 0。
//       页 0-6 是字库点阵，取自 bbk_bios10.nes 的前 7 页；页 7 是汇编结果，起始地址 $C000，空隙填 $FF。
// ------------------------------------------------------------------------

import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { Gasm } from '../../../../machines/bbk/tools/gasm.mjs';
import { BIN_DIR, BIOS_SOURCE, INCLUDE_DIR, INES_HEADER_SIZE, ROM_BIOS10 } from './config.mjs';

/** 一页的字节数 */
const PAGE_SIZE = 0x4000;

/** 文件里的页数：7 页字库加 1 页代码 */
const PAGE_COUNT = 8;

/** 最后一页（代码页）在 CPU 地址空间里的起始地址 */
const CODE_PAGE_BASE = 0xc000;

/** 文件头长度 */
const HEADER_SIZE = 16;

const output = process.argv[2] ?? join(BIN_DIR, 'bios.bbk');
const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(BIOS_SOURCE);
if (result.undefinedSymbols.size > 0) throw new Error(`有未定义符号：${[...result.undefinedSymbols.keys()].join(', ')}`);
const entry = result.symbols.get('begin');
if (entry === undefined) throw new Error('找不到复位入口 Begin');

const image = Buffer.alloc(HEADER_SIZE + PAGE_COUNT * PAGE_SIZE, 0xff);
image.fill(0, 0, HEADER_SIZE);
image.write('BBKB', 0, 'latin1');
image[4] = 1;
image[5] = PAGE_COUNT;
image.writeUInt16LE(entry, 6);

// 字库页取自原版 ROM（跳过 iNES 头）
const original = readFileSync(ROM_BIOS10);
original.copy(image, HEADER_SIZE, INES_HEADER_SIZE, INES_HEADER_SIZE + (PAGE_COUNT - 1) * PAGE_SIZE);

const codeOffset = HEADER_SIZE + (PAGE_COUNT - 1) * PAGE_SIZE;
image.fill(0xff, codeOffset);
for (const [address, byte] of result.memory) {
  if (address < CODE_PAGE_BASE || address >= CODE_PAGE_BASE + PAGE_SIZE) throw new Error(`汇编结果超出代码页：$${address.toString(16)}`);
  image[codeOffset + address - CODE_PAGE_BASE] = byte;
}

mkdirSync(dirname(output), { recursive: true });
writeFileSync(output, image);
const used = result.memory.size;
console.log(`已写入 ${output}（${image.length} 字节），入口 $${entry.toString(16)}，代码页已用 ${used} 字节，剩余 ${PAGE_SIZE - used} 字节`);
