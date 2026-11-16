// ------------------------------------------------------------------------
// 名称：compare-dumps.mjs
// 说明：比较 run-bbk.mjs --dump-line 转存的两份 CPU 内存，列出不同的字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/compare-dumps.mjs <标签甲> <标签乙> <before|after>；每处不同列出地址、甲值、乙值，连续的不同字节合并成一段。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const [labelA, labelB, stage] = process.argv.slice(2);
if (!labelA || !labelB || !stage) {
  console.error('用法：node tools/compare-dumps.mjs <标签甲> <标签乙> <before|after>');
  process.exit(2);
}

/**
 * 读取转存文件，得到“地址 → 字节”的表。
 * @param {string} label 标签
 * @returns {Map<number, number>} 地址到字节
 */
function load(label) {
  const memory = new Map();
  for (const line of readFileSync(join(ROOT, 'build', `dump-${label}-${stage}.txt`), 'utf8').split('\n')) {
    if (line.trim() === '') continue;
    const start = Number.parseInt(line.slice(0, 4), 16);
    const hex = line.slice(5);
    for (let index = 0; index < hex.length; index += 2) memory.set(start + index / 2, Number.parseInt(hex.slice(index, index + 2), 16));
  }
  return memory;
}

const a = load(labelA);
const b = load(labelB);
const differences = [...a.keys()].filter((address) => a.get(address) !== b.get(address)).sort((left, right) => left - right);
const hex2 = (value) => value.toString(16).toUpperCase().padStart(2, '0');
let index = 0;
while (index < differences.length) {
  let last = index;
  while (last + 1 < differences.length && differences[last + 1] === differences[last] + 1) last += 1;
  const range = differences.slice(index, last + 1);
  console.log(`$${differences[index].toString(16).toUpperCase().padStart(4, '0')}${last > index ? `-${differences[last].toString(16).toUpperCase().padStart(4, '0')}` : ''}  甲 ${range.map((address) => hex2(a.get(address))).join(' ')}  乙 ${range.map((address) => hex2(b.get(address))).join(' ')}`);
  index = last + 1;
}
console.log(`共 ${differences.length} 个字节不同`);
