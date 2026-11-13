// ------------------------------------------------------------------------
// 名称：scan-access.mjs
// 说明：扫描 GBASIC 的反汇编结果，列出所有访问 GBASIC 自己范围之外（$2000 以上）的指令，作为移植到 SB2000 的访问点清单。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：用法 node tools/scan-access.mjs [输出文件]；入口与 trace 同 disasm-gbasic.mjs，要先运行 trace-gbasic.mjs。
//       每行：站点地址、助记符、目标地址、区域、前两条指令（看 A、X、Y 的立即数，判断功能号）。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { analyzeCode } from '../../../../packages/fcdev/src/disasm/index.mjs';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { parseTrace } from '../../../../packages/fcdev/src/disasm/trace.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const BASE = 0x7b00;
const ENTRIES = [0x7b00, 0x994d, 0x97f6, 0x94db];
const IMAGE = readFileSync(join(ROOT, 'GBASIC.CMD')).subarray(2);
const END = BASE + IMAGE.length;

/**
 * 地址所属的区域名称。
 * @param {number} address 地址
 * @returns {string} 区域名
 */
function regionOf(address) {
  if (address < 0x2000) return '内部 RAM';
  if (address < 0x4000) return 'PPU';
  if (address < 0x4020) return 'APU/手柄';
  if (address < 0x4400) return '$4020-$43FF';
  if (address < 0x5800) return '$4400-$57FF';
  if (address < 0x6000) return '系统 RAM $5800-$5FFF';
  if (address < BASE) return '公共 RAM $6000-$7AFF';
  if (address >= 0xff00) return '步步高 I/O $FF00-$FFFF';
  return '映像之外 $E410-$FEFF';
}

const traced = parseTrace(readFileSync(join(ROOT, 'build', 'gbasic-trace.txt'), 'utf8'));
const { kinds } = analyzeCode({ bytes: IMAGE, base: BASE, entries: [...ENTRIES, ...traced] });

/** 指令偏移列表，按地址排序，用来找“前两条指令” */
const instructions = [];
for (let offset = 0; offset < IMAGE.length; offset += 1) if (kinds[offset] === 1) instructions.push(offset);

/**
 * 把一条指令格式化成文本。
 * @param {number} offset 偏移
 * @returns {string} 如 "lda #$09"
 */
function describe(offset) {
  const [mnemonic, size] = OPCODES[IMAGE[offset]];
  const operand = size === 1 ? '' : size === 2 ? ` $${IMAGE[offset + 1].toString(16).toUpperCase().padStart(2, '0')}` : ` $${IMAGE.readUInt16LE(offset + 1).toString(16).toUpperCase().padStart(4, '0')}`;
  return `${mnemonic}${operand}`;
}

const rows = [];
instructions.forEach((offset, index) => {
  const [mnemonic, size] = OPCODES[IMAGE[offset]];
  if (size !== 3) return;
  const target = IMAGE.readUInt16LE(offset + 1);
  if (target < 0x2000 || (target >= BASE && target < END)) return;
  const previous = instructions.slice(Math.max(0, index - 2), index).map(describe).join(' ; ');
  rows.push({ site: BASE + offset, mnemonic, target, region: regionOf(target), previous });
});

const hex = (value, width = 4) => value.toString(16).toUpperCase().padStart(width, '0');
const lines = ['站点 | 指令 | 目标 | 区域 | 前两条指令'];
for (const row of rows) lines.push(`$${hex(row.site)} | ${row.mnemonic} | $${hex(row.target)} | ${row.region} | ${row.previous}`);
const summary = new Map();
for (const row of rows) summary.set(row.region, (summary.get(row.region) ?? 0) + 1);
lines.push('', '按区域汇总：');
for (const [region, count] of [...summary].sort()) lines.push(`${region}：${count}`);
lines.push(`合计：${rows.length}`);
const output = process.argv[2] ?? join(ROOT, 'build', 'access-sites.txt');
writeFileSync(output, `${lines.join('\n')}\n`, 'utf8');
console.log(lines.slice(-8).join('\n'));
