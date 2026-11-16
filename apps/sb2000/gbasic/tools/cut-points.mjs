// ------------------------------------------------------------------------
// 名称：cut-points.mjs
// 说明：统计把原版映像按地址切开时，每个候选切点两侧的交叉引用：分支（不能跨块）、jsr、jmp、地址表与立即数引用，供阶段 4 决定多页放置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/cut-points.mjs [--from 8000 --to C127 --step 100]；切点只取 jsr 的目标地址（例程入口）。交叉数只算“代码指令 → 另一侧”与“另一侧 → 本侧”，数据区引用另列。
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const argument = (name, fallback) => {
  const at = process.argv.indexOf(`--${name}`);
  return at >= 0 ? Number.parseInt(process.argv[at + 1], 16) : fallback;
};
const from = argument('from', 0x8000);
const to = argument('to', 0xc127);

const model = loadImageModel();
const { bytes, items } = model;

// 全部“指令 → 目标”的边
const edges = [];
for (const item of items) {
  if (item.type !== 'code') continue;
  const mode = addressingMode(item.opcode);
  const offset = item.start - BASE;
  const name = OPCODES[item.opcode][0];
  if (mode === 'rel') edges.push({ at: item.start, target: (item.start + 2 + bytes.readInt8(offset + 1)) & 0xffff, kind: 'branch' });
  else if (name === 'jsr') edges.push({ at: item.start, target: bytes.readUInt16LE(offset + 1), kind: 'jsr' });
  else if (name === 'jmp' && mode === 'abs') edges.push({ at: item.start, target: bytes.readUInt16LE(offset + 1), kind: 'jmp' });
  else if (['abs', 'abx', 'aby'].includes(mode)) edges.push({ at: item.start, target: bytes.readUInt16LE(offset + 1), kind: 'data' });
}

const entries = [...new Set(edges.filter((edge) => edge.kind === 'jsr' && edge.target >= from && edge.target <= to).map((edge) => edge.target))].sort((left, right) => left - right);
const wanted = process.argv.includes('--all') ? entries : entries.filter((address, index) => index % 12 === 0);
const rows = [];
for (const cut of wanted) {
  const count = { branch: 0, jsr: 0, jmp: 0, data: 0 };
  for (const edge of edges) {
    const sourceLow = edge.at < cut;
    const targetLow = edge.target < cut;
    if (sourceLow === targetLow) continue;
    if (edge.at < from || edge.at > to || edge.target < from || edge.target > to) continue;
    count[edge.kind] += 1;
  }
  rows.push(`$${hex4(cut)}  分支 ${count.branch}  jsr ${count.jsr}  jmp ${count.jmp}  数据 ${count.data}`);
}
console.log(rows.join('\n'));
