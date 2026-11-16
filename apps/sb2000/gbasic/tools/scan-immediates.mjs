// ------------------------------------------------------------------------
// 名称：scan-immediates.mjs
// 说明：在代码里找“立即数里拆开的地址”：相近的两条 LDA/LDX/LDY 立即数指令，一条是某个映像内地址的低字节、另一条是高字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/scan-immediates.mjs [--window <条数>] [--all]；结果分三类：A 拼出的地址已被别处引用，B 拼出的地址是某个条目的起点，C 其余（多半是巧合）。
//       默认只列 A、B，--all 连 C 一起列。这些只是候选，必须逐条看代码确认后写进 analysis/immediates.txt（每行：指令地址 lo|hi 目标地址）。
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, collectTargets, hex2, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const args = process.argv.slice(2);
const windowIndex = args.indexOf('--window');
const WINDOW = windowIndex >= 0 ? Number(args[windowIndex + 1]) : 16;

const model = loadImageModel();
const { bytes, end, items } = model;
const targets = collectTargets(model);
const starts = new Set(items.map((item) => item.start));

const loads = [];
items.forEach((item, index) => {
  if (item.type !== 'code' || addressingMode(item.opcode) !== 'imm' || !['lda', 'ldx', 'ldy'].includes(OPCODES[item.opcode][0])) return;
  loads.push({ index, address: item.start, value: bytes[item.start - BASE + 1], name: OPCODES[item.opcode][0] });
});

const found = [];
for (let first = 0; first < loads.length; first += 1) {
  for (let second = first + 1; second < loads.length && loads[second].index - loads[first].index <= WINDOW; second += 1) {
    for (const [low, high] of [[loads[first], loads[second]], [loads[second], loads[first]]]) {
      const value = (high.value << 8) | low.value;
      if (value < BASE || value >= end) continue;
      const kind = targets.has(value) ? 'A' : starts.has(value) ? 'B' : 'C';
      found.push({ kind, value, low, high });
    }
  }
}
const order = { A: 0, B: 1, C: 2 };
found.sort((left, right) => order[left.kind] - order[right.kind] || left.low.address - right.low.address);
const counts = { A: 0, B: 0, C: 0 };
for (const entry of found) counts[entry.kind] += 1;
console.log(`候选 ${found.length} 个：A ${counts.A}、B ${counts.B}、C ${counts.C}（窗口 ${WINDOW} 条指令）`);

/**
 * 一条指令的字节与助记符，配合 --context 逐条核对。
 * @param {object} item 指令条目
 * @returns {string} 如 7B36 A2 7B   ldx
 */
function describe(item) {
  const text = [...bytes.subarray(item.start - BASE, item.start - BASE + item.size)].map((value) => hex2(value)).join(' ');
  return `${hex4(item.start)}  ${text.padEnd(9)} ${OPCODES[item.opcode][0]}`;
}

for (const entry of found) {
  if (entry.kind === 'C' && !args.includes('--all')) continue;
  if (args.includes('--context')) {
    const from = model.itemIndexAt(Math.min(entry.low.address, entry.high.address));
    const to = model.itemIndexAt(Math.max(entry.low.address, entry.high.address));
    console.log(`--- ${entry.kind} $${hex4(entry.value)}  低@${hex4(entry.low.address)} 高@${hex4(entry.high.address)}`);
    for (let index = Math.max(0, from - 1); index <= to + 1; index += 1) if (items[index].type === 'code') console.log(`   ${describe(items[index])}`);
    continue;
  }
  console.log(`${entry.kind} $${hex4(entry.value)}  ${entry.low.name} #$${hex2(entry.low.value)} @${hex4(entry.low.address)} 低  +  ${entry.high.name} #$${hex2(entry.high.value)} @${hex4(entry.high.address)} 高`);
}
