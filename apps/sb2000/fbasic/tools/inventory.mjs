// ------------------------------------------------------------------------
// 名称：inventory.mjs
// 说明：按例程列出 Family BASIC 里依赖 FC 机器的访问点：PPU 寄存器、声音寄存器、$4016/$4017 口、$6000-$7FFF 程序存储器，以及对它们的调用关系。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/inventory.mjs [--callers]；例程的起点取被 JSR 调用的地址、地址表指向的地址与代码入口；--callers 另列每个例程的调用者数。
// ------------------------------------------------------------------------
import { OPCODES } from './lib/opcodes.mjs';
import { BASE, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const model = loadImageModel();
const { bytes, items } = model;
const starts = new Set();
const callers = new Map();
for (const item of items) {
  if (item.type !== 'code') continue;
  const mnemonic = OPCODES[item.opcode][0];
  if (mnemonic === 'jsr') {
    const target = bytes.readUInt16LE(item.start - BASE + 1);
    starts.add(target);
    callers.set(target, (callers.get(target) ?? 0) + 1);
  }
}
for (const range of model.layout) {
  if (range.kind !== 'addrs') continue;
  for (let address = range.start; address + 1 <= range.end; address += 2) {
    const target = bytes.readUInt16LE(address - BASE);
    if (target >= BASE && target < 0xccab) starts.add(target);
  }
}
const sortedStarts = [...starts].filter((address) => address >= BASE && address < 0xccab).sort((left, right) => left - right);
const routineOf = (address) => {
  let low = 0;
  let high = sortedStarts.length - 1;
  let found = sortedStarts[0];
  while (low <= high) {
    const middle = (low + high) >> 1;
    if (sortedStarts[middle] <= address) { found = sortedStarts[middle]; low = middle + 1; } else high = middle - 1;
  }
  return found;
};

const kindOf = (target) => {
  if (target >= 0x2000 && target <= 0x2007) return 'PPU';
  if (target >= 0x4000 && target <= 0x4015) return 'APU';
  if (target === 0x4016 || target === 0x4017) return 'IO';
  if (target >= 0x4018 && target < 0x6000) return 'IO2';
  if (target >= 0x6000 && target <= 0x7fff) return 'WRAM';
  if (target === 0x4014) return 'DMA';
  return null;
};
const table = new Map();
for (const item of items) {
  if (item.type !== 'code') continue;
  const mode = addressingMode(item.opcode);
  if (!['abs', 'abx', 'aby'].includes(mode)) continue;
  const mnemonic = OPCODES[item.opcode][0];
  if (mnemonic === 'jsr' || mnemonic === 'jmp') continue;
  const target = bytes.readUInt16LE(item.start - BASE + 1);
  const kind = kindOf(target);
  if (kind === null) continue;
  const routine = routineOf(item.start);
  if (!table.has(routine)) table.set(routine, new Map());
  const entry = table.get(routine);
  const label = `${kind === 'PPU' || kind === 'APU' || kind === 'IO' || kind === 'IO2' || kind === 'DMA' ? `$${hex4(target)}` : kind}`;
  entry.set(label, (entry.get(label) ?? 0) + 1);
}
const withCallers = process.argv.includes('--callers');
for (const [routine, entry] of [...table].sort((left, right) => left[0] - right[0])) {
  console.log(`$${hex4(routine)}${withCallers ? `（${callers.get(routine) ?? 0} 处调用）` : ''}：${[...entry].map(([label, count]) => `${label}×${count}`).join(' ')}`);
}
console.log(`共 ${table.size} 个例程`);
