// ------------------------------------------------------------------------
// 名称：gen-keymap.mjs
// 说明：生成 SB2000 键盘到原版键盘矩阵的映射表（exe/src/keymap.s 与 keymap.inc）：按“字符”映射，先把 PC 键（扫描码集 1，按美式键位）翻成字符，再在原版的键码表里找产生这个字符的键与是否要按 SHIFT。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/gen-keymap.mjs；键位矩阵的编号 = 行 × 8 + 位（与原版 $0634-$063C 一致，位 0 是最低位）；表项 $FF 表示没有对应的键，位 7 为 1 表示还要同时按 SHIFT。
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { readRom } from './lib/rom.mjs';
import { ROOT } from './lib/image-model.mjs';

const { prg } = readRom();
const at = (address) => prg[address - 0x8000];
const NORMAL = 0xbaa9;
const SHIFTED = 0xbaf1;
const KEYS = 72;

/** 按字符码找原版键：普通表里有就不用 SHIFT，SHIFT 表里有就要 SHIFT；找不到返回 0xFF。 */
function find(code) {
  if (code === undefined) return 0xff;
  for (let index = 0; index < KEYS; index += 1) if (at(NORMAL + index) === code) return index;
  for (let index = 0; index < KEYS; index += 1) if (at(SHIFTED + index) === code) return index | 0x80;
  return 0xff;
}
const char = (value) => value.charCodeAt(0);
/** 字母键：原版普通表里是大写字母，PC 上有没有按 SHIFT 都对应同一个键。 */
const letter = (value) => [char(value.toUpperCase()), char(value.toUpperCase())];

// 扫描码集 1 的主键区：[普通字符码, 按住 SHIFT 的字符码]
const plain = new Map();
const set = (scan, normal, shifted) => plain.set(scan, [normal, shifted]);
'1234567890'.split('').forEach((digit, index) => set(0x02 + index, char(digit), char(')!@#$%^&*('[(index + 1) % 10])));
set(0x0c, char('-'), char('_'));
set(0x0d, char('='), char('+'));
set(0x0e, 0x08, 0x08);
[...'qwertyuiop'].forEach((value, index) => set(0x10 + index, ...letter(value)));
set(0x1a, char('['), char('{'));
set(0x1b, char(']'), char('}'));
set(0x1c, 0x0d, 0x0d);
[...'asdfghjkl'].forEach((value, index) => set(0x1e + index, ...letter(value)));
set(0x27, char(';'), char(':'));
set(0x28, char("'"), char('"'));
set(0x29, char('`'), char('~'));
set(0x2b, char('\\'), char('|'));
[...'zxcvbnm'].forEach((value, index) => set(0x2c + index, ...letter(value)));
set(0x33, char(','), char('<'));
set(0x34, char('.'), char('>'));
set(0x35, char('/'), char('?'));
set(0x39, char(' '), char(' '));
set(0x01, 0x1b, 0x1b);
for (let number = 0; number < 8; number += 1) set(0x3b + number, 0xf0 + number, 0xf0 + number);

// 扩展键（前缀 $E0）
const extended = [
  [0x48, 0x1e, 0x1e], [0x50, 0x1f, 0x1f], [0x4b, 0x1c, 0x1c], [0x4d, 0x1d, 0x1d],
  [0x52, 0x12, 0x12], [0x53, 0x08, 0x08], [0x47, 0x0b, 0x0c], [0x4f, 0x03, 0x03], [0x1c, 0x0d, 0x0d],
];

const KEYMAP_COUNT = 0x3b + 8;
const row = (values) => `        .byte   ${values.map((value) => `$${value.toString(16).toUpperCase().padStart(2, '0')}`).join(',')}`;
const table = (select) => {
  const lines = [];
  for (let from = 0; from < KEYMAP_COUNT; from += 16) {
    const values = [];
    for (let scan = from; scan < Math.min(from + 16, KEYMAP_COUNT); scan += 1) values.push(plain.has(scan) ? find(plain.get(scan)[select]) : 0xff);
    lines.push(row(values));
  }
  return lines;
};
const header = (name, text) => [
  '; ------------------------------------------------------------------------',
  `; 名称：${name}`,
  `; 说明：${text}`,
  '; 作者：sungbin',
  '; 邮箱：sungbin@3578.cn',
  '; 日期：2026-11-16',
  '; 备注：由 tools/gen-keymap.mjs 生成，不要手改。',
  '; ------------------------------------------------------------------------',
];
const source = [
  ...header('keymap.s', 'SB2000 键盘扫描码到原版键盘矩阵键的映射表。'),
  '        .export         keymap_normal, keymap_shifted, ext_scan, ext_normal, ext_shifted',
  '        .segment        "BANK12"',
  '; 主键区：按扫描码下标；$FF 无对应键，位 7 为 1 要同时按 SHIFT',
  'keymap_normal:',
  ...table(0),
  'keymap_shifted:',
  ...table(1),
  '; 扩展键：扫描码与两张表',
  'ext_scan:',
  row(extended.map((entry) => entry[0])),
  'ext_normal:',
  row(extended.map((entry) => find(entry[1]))),
  'ext_shifted:',
  row(extended.map((entry) => find(entry[2]))),
  '',
];
const include = [
  ...header('keymap.inc', '键盘映射表的大小常量。').map((line) => line.replace(/^; /, '; ')),
  `KEYMAP_COUNT     = ${KEYMAP_COUNT}`,
  `EXT_COUNT        = ${extended.length}`,
  '',
];
writeFileSync(join(ROOT, 'exe', 'src', 'keymap.s'), source.join('\n'));
writeFileSync(join(ROOT, 'exe', 'src', 'keymap.inc'), include.join('\n'));
const unmapped = [...plain].filter(([, [normal, shifted]]) => find(normal) === 0xff || find(shifted) === 0xff).map(([scan, [normal, shifted]]) => `$${scan.toString(16)}(${String.fromCharCode(normal)}${String.fromCharCode(shifted)})`);
console.log(`已写入 exe/src/keymap.s 与 keymap.inc；找不到原版键的：${unmapped.join(' ')}`);
