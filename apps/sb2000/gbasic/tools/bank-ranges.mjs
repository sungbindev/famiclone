// ------------------------------------------------------------------------
// 名称：bank-ranges.mjs
// 说明：在原版代码区里找适合放进换页窗口的地址区间：区间内的代码被区间外引用的入口越少越好，跨出区间的分支为 0。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/bank-ranges.mjs [--size 1000]（十六进制，区间最大字节数）；起点与终点都取例程入口（jsr 目标）或数据区边界。入口数 = 被区间外的 jsr/jmp/地址表引用的不同目标数。
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, dataElements, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const at = process.argv.indexOf('--size');
const size = at >= 0 ? Number.parseInt(process.argv[at + 1], 16) : 0x1000;
const minAt = process.argv.indexOf('--min');
const minimum = minAt >= 0 ? Number.parseInt(process.argv[minAt + 1], 16) : 0;
const from = 0x8000;
const to = 0xc128;

const model = loadImageModel();
const { bytes, items } = model;

const edges = [];
for (const item of items) {
  if (item.type === 'code') {
    const mode = addressingMode(item.opcode);
    const offset = item.start - BASE;
    const name = OPCODES[item.opcode][0];
    if (mode === 'rel') edges.push({ at: item.start, target: (item.start + 2 + bytes.readInt8(offset + 1)) & 0xffff, kind: 'branch' });
    else if (name === 'jsr') edges.push({ at: item.start, target: bytes.readUInt16LE(offset + 1), kind: 'call' });
    else if (name === 'jmp' && mode === 'abs') edges.push({ at: item.start, target: bytes.readUInt16LE(offset + 1), kind: 'call' });
    else if (['abs', 'abx', 'aby'].includes(mode)) edges.push({ at: item.start, target: bytes.readUInt16LE(offset + 1), kind: 'data' });
  } else if (item.type === 'data' && (item.range.kind === 'addrs' || item.range.kind === 'rec')) {
    for (const element of dataElements(item.range)) if (element.kind === 'a') edges.push({ at: element.start, target: bytes.readUInt16LE(element.start - BASE), kind: 'table' });
  }
}

const starts = [...new Set(edges.filter((edge) => edge.kind === 'call' && edge.target >= from && edge.target < to).map((edge) => edge.target))].sort((left, right) => left - right);
const results = [];
for (const low of starts) {
  for (const high of [...starts, to].filter((value) => value > low && value - low <= size && value - low >= minimum)) {
    let branchCross = 0;
    const entryTargets = new Set();
    let dataIn = 0;
    for (const edge of edges) {
      const sourceIn = edge.at >= low && edge.at < high;
      const targetIn = edge.target >= low && edge.target < high;
      if (sourceIn === targetIn) continue;
      if (edge.kind === 'branch' && (sourceIn || targetIn)) branchCross += 1;
      else if (!sourceIn && targetIn) {
        if (edge.kind === 'data') dataIn += 1;
        else entryTargets.add(edge.target);
      }
    }
    if (branchCross > 0 || dataIn > 0) continue;
    results.push({ low, high, bytes: high - low, entries: entryTargets.size });
  }
}
// 每个字节数档里入口最少的前几个
results.sort((left, right) => (left.entries / left.bytes) - (right.entries / right.bytes) || right.bytes - left.bytes);
for (const row of results.slice(0, 40)) console.log(`$${hex4(row.low)}-$${hex4(row.high - 1)}  ${row.bytes} 字节  入口 ${row.entries}`);
