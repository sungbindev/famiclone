// ------------------------------------------------------------------------
// 名称：chunks.mjs
// 说明：把分类后的条目切成可以独立搬动的块：块的末尾是无条件跳转、返回或数据，且没有相对分支跨过块的边界；统计块之间的调用、跳转与地址表引用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：重排测试（lift.mjs --permute）已经证明这样切出来的块可以任意重排；阶段 4 用同样的块分配到常驻页与各库页。
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, dataElements, loadImageModel } from './image-model.mjs';
import { addressingMode } from './opcode-modes.mjs';

const TERMINATORS = ['jump', 'jump-indirect', 'return', 'stop'];

/**
 * 切块并统计块间引用。
 * @returns {{model: object, chunks: {id: number, first: number, last: number, start: number, end: number, size: number, kind: 'code'|'data'}[], chunkAt: (address: number) => number, edges: Map<string, {from: number, to: number, jsr: number, jmp: number, table: number}>}} 结果
 */
export function buildChunks() {
  const model = loadImageModel();
  const { items, bytes, itemIndexAt } = model;
  const blocked = new Set();
  items.forEach((item, index) => {
    if (item.type !== 'code' || addressingMode(item.opcode) !== 'rel') return;
    const target = itemIndexAt((item.start + 2 + bytes.readInt8(item.start - BASE + 1)) & 0xffff);
    for (let position = Math.min(index, target) + 1; position <= Math.max(index, target); position += 1) blocked.add(position);
  });
  const cuts = [0];
  for (let index = 1; index < items.length; index += 1) {
    const previous = items[index - 1];
    const ends = previous.type === 'data' || (previous.type === 'code' && TERMINATORS.includes(OPCODES[previous.opcode][2]));
    if (ends && !blocked.has(index)) cuts.push(index);
  }
  const chunks = cuts.map((first, id) => {
    const last = (cuts[id + 1] ?? items.length) - 1;
    const start = items[first].start;
    const end = items[last].start + items[last].size - 1;
    const kind = items.slice(first, last + 1).every((item) => item.type === 'data') ? 'data' : 'code';
    return { id, first, last, start, end, size: end - start + 1, kind };
  });
  const chunkOfItem = new Array(items.length);
  chunks.forEach((chunk) => { for (let index = chunk.first; index <= chunk.last; index += 1) chunkOfItem[index] = chunk.id; });
  const chunkAt = (address) => chunkOfItem[itemIndexAt(address)];

  const edges = new Map();
  const add = (from, to, field) => {
    if (from === to) return;
    const key = `${from}>${to}`;
    if (!edges.has(key)) edges.set(key, { from, to, jsr: 0, jmp: 0, table: 0 });
    edges.get(key)[field] += 1;
  };
  items.forEach((item, index) => {
    const from = chunkOfItem[index];
    if (item.type === 'code') {
      const mnemonic = OPCODES[item.opcode][0];
      if (mnemonic !== 'jsr' && mnemonic !== 'jmp') return;
      const target = bytes.readUInt16LE(item.start - BASE + 1);
      if (target < BASE || target >= BASE + bytes.length) return;
      add(from, chunkAt(target), mnemonic === 'jsr' ? 'jsr' : 'jmp');
    } else if (item.type === 'data' && item.range.kind === 'addrs') {
      for (const element of dataElements(item.range).filter((entry) => entry.start >= item.start && entry.start < item.start + item.size)) {
        const target = bytes.readUInt16LE(element.start - BASE);
        if (target >= BASE && target < BASE + bytes.length) add(from, chunkAt(target), 'table');
      }
    }
  });
  return { model, chunks, chunkAt, edges };
}
