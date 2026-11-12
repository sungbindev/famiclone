// ------------------------------------------------------------------------
// 名称：match-rom.mjs
// 说明：把汇编结果里“位置无关”的指令按每 N 条一组，在原厂二进制里整段查找，
//       用来判断源码与二进制是否属于同一代码、汇编器的编码是否正确。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/match-rom.mjs <源文件> <二进制文件|--rom> [每组条数，缺省 6]
//       位置无关指令指不含绝对地址操作数的指令（立即数、零页、相对分支、无操作数），
//       源码与二进制的构建不同、地址整体偏移时，这类指令的字节仍然相同。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import { Gasm } from './gasm.mjs';
import { INCLUDE_DIR, ROM_BIOS10, resolveSource } from './config.mjs';

const [sourceName, binaryArg, groupArg] = process.argv.slice(2);
if (!sourceName || !binaryArg) {
  console.error('用法：node tools/match-rom.mjs <源文件> <二进制文件|--rom> [每组条数]');
  process.exit(2);
}

const groupSize = Number(groupArg ?? 6);
const binary = readFileSync(binaryArg === '--rom' ? ROM_BIOS10 : binaryArg);
const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(resolveSource(sourceName));

let total = 0;
let found = 0;
let run = [];

/** 把当前连续的位置无关指令切成整组并查找，不足一组的尾部丢弃 */
function flush() {
  for (let start = 0; start + groupSize <= run.length; start += groupSize) {
    const bytes = Buffer.from(run.slice(start, start + groupSize).flatMap((entry) => entry.bytes));
    total += 1;
    if (binary.indexOf(bytes) >= 0) found += 1;
  }
  run = [];
}

for (const entry of result.listing) {
  // 三字节指令带绝对地址，不是位置无关的
  if (entry.kind === 'ins' && entry.bytes.length < 3) run.push(entry);
  else flush();
}
flush();

const percent = total ? Math.round((found * 100) / total) : 0;
console.log(`${sourceName}：${total} 组位置无关指令（每组 ${groupSize} 条），在二进制里找到 ${found} 组（${percent}%）`);
