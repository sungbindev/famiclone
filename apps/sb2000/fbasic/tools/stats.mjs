// ------------------------------------------------------------------------
// 名称：stats.mjs
// 说明：统计分类结果与硬件访问点：代码与各类数据的字节数、指令条数，对 PPU、声音、手柄/键盘口、$6000-$7FFF 程序存储器的访问次数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/stats.mjs；阶段 1 结束时的总量与阶段 2 的依赖清单以它为准。
// ------------------------------------------------------------------------
import { OPCODES } from './lib/opcodes.mjs';
import { BASE, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const model = loadImageModel();
const { bytes, items } = model;
let instructions = 0;
let codeBytes = 0;
const dataBytes = {};
const access = new Map();
const note = (key, address) => { if (!access.has(key)) access.set(key, []); access.get(key).push(address); };
for (const item of items) {
  if (item.type === 'data') { dataBytes[item.range.kind] = (dataBytes[item.range.kind] ?? 0) + item.size; continue; }
  codeBytes += item.size;
  if (item.type === 'skip') continue;
  instructions += 1;
  const mode = addressingMode(item.opcode);
  if (!['abs', 'abx', 'aby', 'ind'].includes(mode)) continue;
  const target = bytes.readUInt16LE(item.start - BASE + 1);
  const mnemonic = OPCODES[item.opcode][0];
  if (mnemonic === 'jsr' || mnemonic === 'jmp') continue;
  if (target >= 0x2000 && target <= 0x2007) note(`PPU $${hex4(target)}`, item.start);
  else if (target >= 0x4000 && target <= 0x4015) note('声音 $4000-$4015', item.start);
  else if (target === 0x4016 || target === 0x4017) note(`手柄/键盘口 $${hex4(target)}`, item.start);
  else if (target >= 0x4018 && target < 0x6000) note('其他 $4018-$5FFF', item.start);
  else if (target >= 0x6000 && target <= 0x7fff) note('程序存储器 $6000-$7FFF', item.start);
}
console.log(`PRG ${bytes.length} 字节：代码 ${codeBytes}（${instructions} 条指令，其中跳过前缀 ${items.filter((item) => item.type === 'skip').length} 处），数据 ${bytes.length - codeBytes}`);
console.log(`数据按类型：${Object.entries(dataBytes).map(([kind, size]) => `${kind} ${size}`).join('，')}；${model.layout.filter((range) => range.implicit).length} 段未细分的数据`);
console.log(`立即数里拆开的地址：${model.immediates.size} 条指令`);
const zeroPage = new Set();
const ramPages = new Map();
for (const item of items) {
  if (item.type !== 'code') continue;
  const mode = addressingMode(item.opcode);
  const mnemonic = OPCODES[item.opcode][0];
  if (['zp', 'zpx', 'zpy', 'izx', 'izy'].includes(mode)) zeroPage.add(bytes[item.start - BASE + 1]);
  else if (['abs', 'abx', 'aby'].includes(mode) && mnemonic !== 'jsr' && mnemonic !== 'jmp') {
    const target = bytes.readUInt16LE(item.start - BASE + 1);
    if (target >= 0x100 && target < 0x800) { const page = target >> 8; if (!ramPages.has(page)) ramPages.set(page, new Set()); ramPages.get(page).add(target); }
  }
}
console.log(`零页：用到 ${zeroPage.size} 个单元；$0100-$07FF 按页：${[...ramPages].sort((left, right) => left[0] - right[0]).map(([page, set]) => `$${page.toString(16).toUpperCase().padStart(2, '0')}xx ${set.size} 个`).join('，')}`);
for (const [key, list] of [...access].sort((left, right) => left[0].localeCompare(right[0]))) console.log(`${key}：${list.length} 处`);
