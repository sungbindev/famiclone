// ------------------------------------------------------------------------
// 名称：extract-seeds.mjs
// 说明：从 analysis/prg.cdl 取出原版 Family BASIC 执行过的指令起点，写成 analysis/cdl-entries.txt，作为代码发现的种子。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/extract-seeds.mjs；CDL 由 tools/record-coverage.mjs 记录。起点取：每段连续代码字节的第一个字节、
//       被标成跳转目标或子程序入口的代码字节，以及 RESET 向量；起点处的字节必须与 PRG 里的操作码相符才采用。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, ROOT, readRom } from './lib/image-model.mjs';

const CODE = 1;
const JUMP_TARGET = 4;
const SUB_ENTRY = 8;

const { prg } = readRom();
const cdl = readFileSync(join(ROOT, 'analysis', 'prg.cdl'));
const entries = new Set([prg.readUInt16LE(0x7ffc)]);
for (let offset = 0; offset < prg.length; offset += 1) {
  const flags = cdl[offset];
  if ((flags & CODE) === 0) continue;
  const startsRun = offset === 0 || (cdl[offset - 1] & CODE) === 0;
  if (startsRun || (flags & (JUMP_TARGET | SUB_ENTRY)) !== 0) entries.add(BASE + offset);
}
const sorted = [...entries].sort((left, right) => left - right).filter((address) => OPCODES[prg[address - BASE]] !== null);
const lines = ['# 原版运行时 CDL 里的指令起点（每行一个，十六进制）：连续代码段的起点、跳转目标、子程序入口与 RESET 向量', ...sorted.map((address) => address.toString(16).toUpperCase().padStart(4, '0'))];
writeFileSync(join(ROOT, 'analysis', 'cdl-entries.txt'), `${lines.join('\n')}\n`);
console.log(`写入 ${sorted.length} 个起点`);
