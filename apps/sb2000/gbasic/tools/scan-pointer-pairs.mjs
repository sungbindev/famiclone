// ------------------------------------------------------------------------
// 名称：scan-pointer-pairs.mjs
// 说明：找“立即数加零页指针”的写法：两条立即数加载指令把一个映像内地址的低、高字节分别存进相邻的零页单元（lda #低 / sta zp / lda #高 / sta zp+1，寄存器可以是 A、X、Y）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/scan-pointer-pairs.mjs [--window <条数>] [--write]；这种写法几乎一定是地址，所以不需要逐条人工确认，但输出要扫一眼；
//       --write 把结果并入 analysis/immediates.txt（已有的行保留）。立即数加载之后 6 条指令内、寄存器没被改动前遇到的第一条零页存储才算它的去向。
// ------------------------------------------------------------------------
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, ROOT, hex2, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const args = process.argv.slice(2);
const windowIndex = args.indexOf('--window');
const WINDOW = windowIndex >= 0 ? Number(args[windowIndex + 1]) : 10;
const LOOKAHEAD = 6;

const model = loadImageModel();
const { bytes, end, items } = model;

const REGISTER_STORE = { sta: 'lda', stx: 'ldx', sty: 'ldy' };
const CHANGES_REGISTER = {
  lda: ['lda', 'txa', 'tya', 'pla', 'adc', 'sbc', 'and', 'ora', 'eor', 'asl', 'lsr', 'rol', 'ror', 'jsr'],
  ldx: ['ldx', 'tax', 'tsx', 'inx', 'dex', 'jsr'],
  ldy: ['ldy', 'tay', 'iny', 'dey', 'jsr'],
};

// 每条立即数加载：它的值最终被存进哪个零页单元
const stored = [];
items.forEach((item, index) => {
  if (item.type !== 'code' || addressingMode(item.opcode) !== 'imm') return;
  const mnemonic = OPCODES[item.opcode][0];
  if (!['lda', 'ldx', 'ldy'].includes(mnemonic)) return;
  for (let ahead = index + 1; ahead <= index + LOOKAHEAD && ahead < items.length; ahead += 1) {
    const next = items[ahead];
    if (next.type !== 'code') break;
    const nextName = OPCODES[next.opcode][0];
    if (REGISTER_STORE[nextName] === mnemonic && addressingMode(next.opcode) === 'zp') {
      stored.push({ index, address: item.start, value: bytes[item.start - BASE + 1], zp: bytes[next.start - BASE + 1], register: mnemonic });
      break;
    }
    if (CHANGES_REGISTER[mnemonic].includes(nextName) || OPCODES[next.opcode][2] !== 'next') break;
  }
});

// 低字节与高字节互为最近的一对才算（避免把相邻两处写法里的低、高字节交叉配对）；
// 目标低于 $8800 的不自动采用：$8000-$87FF 一带在步步高机器上也是分页窗口里的数据地址，要人工看是映像标签还是分页内存地址
const AUTO_MINIMUM = 0x8800;
const nearest = (from, candidates) => candidates.reduce((best, candidate) => (best === null || Math.abs(candidate.index - from.index) < Math.abs(best.index - from.index) ? candidate : best), null);
const pairs = [];
const skipped = [];
for (const low of stored) {
  const highs = stored.filter((high) => high !== low && high.zp === low.zp + 1 && Math.abs(high.index - low.index) <= WINDOW);
  const high = nearest(low, highs);
  if (high === null) continue;
  const lows = stored.filter((candidate) => candidate !== high && candidate.zp + 1 === high.zp && Math.abs(candidate.index - high.index) <= WINDOW);
  if (nearest(high, lows) !== low) continue;
  const value = (high.value << 8) | low.value;
  if (value < BASE || value >= end) continue;
  (value < AUTO_MINIMUM ? skipped : pairs).push({ value, low, high });
}
pairs.sort((left, right) => left.low.address - right.low.address);
for (const pair of pairs) {
  console.log(`$${hex4(pair.value)}  ${pair.low.register} #$${hex2(pair.low.value)} @${hex4(pair.low.address)} → $${hex2(pair.low.zp)}  +  ${pair.high.register} #$${hex2(pair.high.value)} @${hex4(pair.high.address)} → $${hex2(pair.high.zp)}`);
}
console.log(`共 ${pairs.length} 对；另有 ${skipped.length} 对目标低于 $${hex4(AUTO_MINIMUM)}，未采用：${skipped.map((pair) => `$${hex4(pair.value)}@${hex4(pair.low.address)}`).join(' ')}`);

if (args.includes('--write')) {
  const file = join(ROOT, 'analysis', 'immediates.txt');
  const existing = existsSync(file) ? readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line.trim() !== '') : ['# 立即数里拆开的地址：每行 “指令地址 lo|hi 目标地址[+偏移]”，由 scan-pointer-pairs.mjs 与人工复核写入'];
  const seen = new Set(existing.map((line) => line.split(/\s+/)[0].toUpperCase()));
  let added = 0;
  for (const pair of pairs) {
    for (const [side, half] of [[pair.low, 'lo'], [pair.high, 'hi']]) {
      const key = hex4(side.address);
      if (seen.has(key)) continue;
      seen.add(key);
      existing.push(`${key} ${half} ${hex4(pair.value)}  # 零页指针 $${hex2(side.zp)}`);
      added += 1;
    }
  }
  writeFileSync(file, `${existing.join('\n')}\n`);
  console.log(`已并入 ${added} 行到 analysis/immediates.txt`);
}
