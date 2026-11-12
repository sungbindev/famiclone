// ------------------------------------------------------------------------
// 名称：build-nes.mjs
// 说明：把原厂 Bios.asm 汇编后放进 128KB 的 BIOS ROM 映像（iNES 格式），供模拟器加载。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/build-nes.mjs [输出文件]，缺省输出 build/bbk_src.nes。
//       ROM 前 7 页（字库点阵）取自 bbk_bios10.nes，最后一页（$C000-$FFFF）用汇编结果，
//       iNES 头沿用 bbk_bios10.nes；汇编结果里没有内容的地址填 $FF。
// ------------------------------------------------------------------------

import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { Gasm } from './gasm.mjs';
import { BUILD_DIR, INCLUDE_DIR, INES_HEADER_SIZE, ORIGINAL_SOURCE_DIR, ROM_BIOS10 } from './config.mjs';

/** 一页的字节数 */
const BANK_SIZE = 0x4000;

/** 最后一页在 CPU 地址空间里的起始地址 */
const LAST_BANK_BASE = 0xc000;

const output = process.argv[2] ?? join(BUILD_DIR, 'bbk_src.nes');
const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(join(ORIGINAL_SOURCE_DIR, 'Bios.asm'));
if (result.undefinedSymbols.size > 0) throw new Error(`有未定义符号：${[...result.undefinedSymbols.keys()].join(', ')}`);

const rom = Buffer.from(readFileSync(ROM_BIOS10));
const lastBankOffset = rom.length - BANK_SIZE;
if (lastBankOffset <= INES_HEADER_SIZE) throw new Error('原版 ROM 太小');
rom.fill(0xff, lastBankOffset);
for (const [address, byte] of result.memory) {
  if (address < LAST_BANK_BASE || address >= LAST_BANK_BASE + BANK_SIZE) throw new Error(`汇编结果超出最后一页：$${address.toString(16)}`);
  rom[lastBankOffset + address - LAST_BANK_BASE] = byte;
}

mkdirSync(dirname(output), { recursive: true });
writeFileSync(output, rom);
const vector = (offset) => rom[lastBankOffset + offset] | (rom[lastBankOffset + offset + 1] << 8);
console.log(`已写入 ${output}（${rom.length} 字节），NMI=$${vector(0x3ffa).toString(16)} RESET=$${vector(0x3ffc).toString(16)} IRQ=$${vector(0x3ffe).toString(16)}`);
