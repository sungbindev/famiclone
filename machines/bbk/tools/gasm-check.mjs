// ------------------------------------------------------------------------
// 名称：gasm-check.mjs
// 说明：汇编一个原厂源文件，列出错误、找不到的包含文件、未定义符号、不认识的宏与重复定义。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/gasm-check.mjs Bios.asm；文件名相对原厂源码目录。
// ------------------------------------------------------------------------

import { join } from 'node:path';
import { Gasm } from './gasm.mjs';
import { INCLUDE_DIR, ORIGINAL_SOURCE_DIR } from './config.mjs';

const name = process.argv[2] ?? 'Bios.asm';
const gasm = new Gasm({ includeDirs: [INCLUDE_DIR] });
const result = gasm.assemble(join(ORIGINAL_SOURCE_DIR, name));

const addresses = [...result.memory.keys()];
console.log(`${name}: ${result.memory.size} 字节，地址 $${Math.min(...addresses).toString(16)}-$${Math.max(...addresses).toString(16)}`);
console.log(`缺失的包含文件：${result.missingIncludes.map((item) => item.name).join(', ') || '无'}`);
console.log(`重复定义（值不同）：${result.duplicates.size}`);
for (const item of result.duplicates.values()) console.log(`  ${item.name} @ ${item.file}(${item.line})`);
console.log(`不认识的助记符或宏：${result.unknownOps.size}`);
for (const item of result.unknownOps.values()) console.log(`  ${item.name}  首次出现 行 ${item.line}`);
console.log(`未定义符号：${result.undefinedSymbols.size}`);
for (const item of [...result.undefinedSymbols.values()].sort((a, b) => a.name.localeCompare(b.name))) {
  console.log(`  ${item.name}  首次引用 行 ${item.line}`);
}
