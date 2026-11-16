// ------------------------------------------------------------------------
// 名称：inventory.mjs
// 说明：列出 GBASIC 里全部依赖步步高机器的地方：对硬件寄存器的访问与对 BBGDOS、BIOS、外设、文件服务的调用，按所在例程分组，供写移植对照表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/inventory.mjs [--md]；所在例程取“被 jsr 调用的最近的前一个标签”，只是定位用的近似；功能号取调用前三条指令内的 lda #立即数，找不到写“?”。
//       文件服务的包装例程 L7B33（A=功能号，参数块 L7B03）的调用也列在“文件”里。
// ------------------------------------------------------------------------
import { OPCODES } from '../../../../packages/fcdev/src/disasm/opcode-table.mjs';
import { BASE, collectTargets, hex2, hex4, loadImageModel } from './lib/image-model.mjs';
import { addressingMode } from './lib/opcode-modes.mjs';

const model = loadImageModel();
const { bytes, items, symbols } = model;
const targets = collectTargets(model);

// 例程起点：被 jsr 调用的目标
const routineStarts = new Set();
for (const item of items) {
  if (item.type === 'code' && item.opcode === 0x20) routineStarts.add(bytes.readUInt16LE(item.start - BASE + 1));
}
const routineOf = (index) => {
  for (let at = index; at >= 0; at -= 1) if (routineStarts.has(items[at].start)) return items[at].start;
  return 0;
};

const external = (address) => (address >= 0x2000 && address < 0x2008) || (address >= 0x4000 && address < 0x4020) || (address >= 0x5800 && address < 0x6000) || (address >= 0xff00 && address <= 0xffff);
const SERVICE = { 0x5800: 'BIOS', 0x5803: '文件', 0x5809: '外设', 0x580c: 'DOS' };
const WRAPPERS = new Map([[0x7b33, '文件'], [0x7d27, 'BIOS 设页(A→X)']]);

const rows = [];
items.forEach((item, index) => {
  if (item.type !== 'code') return;
  const mode = addressingMode(item.opcode);
  if (!['abs', 'abx', 'aby', 'ind'].includes(mode)) return;
  const target = bytes.readUInt16LE(item.start - BASE + 1);
  const name = OPCODES[item.opcode][0];
  let function_ = '';
  if ((name === 'jsr' || name === 'jmp') && (SERVICE[target] || WRAPPERS.has(target))) {
    for (let back = index - 1; back >= Math.max(0, index - 4); back -= 1) {
      const previous = items[back];
      if (previous.type === 'code' && previous.opcode === 0xa9) { function_ = `A=$${hex2(bytes[previous.start - BASE + 1])}`; break; }
    }
    rows.push({ routine: routineOf(index), at: item.start, kind: SERVICE[target] ?? WRAPPERS.get(target), detail: function_ || 'A=?' });
  } else if (external(target) && !SERVICE[target]) {
    rows.push({ routine: routineOf(index), at: item.start, kind: '硬件', detail: `${name} $${hex4(target)}${mode === 'abx' ? ',x' : mode === 'aby' ? ',y' : ''}` });
  }
});

const groups = new Map();
for (const row of rows) {
  if (!groups.has(row.routine)) groups.set(row.routine, []);
  groups.get(row.routine).push(row);
}
void targets;
void symbols;
const lines = [];
for (const [routine, list] of [...groups].sort((left, right) => left[0] - right[0])) {
  const summary = new Map();
  for (const row of list) {
    const key = `${row.kind} ${row.detail}`;
    if (!summary.has(key)) summary.set(key, []);
    summary.get(key).push(hex4(row.at));
  }
  lines.push(`$${hex4(routine)}（${list.length} 处）：${[...summary].map(([key, at]) => `${key}@${at.slice(0, 3).join(',')}${at.length > 3 ? `等${at.length}` : ''}`).join('；')}`);
}
console.log(lines.join('\n'));
console.log(`合计 ${rows.length} 处，${groups.size} 个例程`);

if (process.argv.includes('--ram')) {
  // 代码对 RAM 的访问：零页按单元、其余按 256 字节一页，统计被访问的不同地址数与访问次数
  const zeroPage = new Map();
  const pages = new Map();
  for (const item of items) {
    if (item.type !== 'code') continue;
    const mode = addressingMode(item.opcode);
    const offset = item.start - BASE;
    let address = -1;
    if (['zp', 'zpx', 'zpy', 'izx', 'izy'].includes(mode)) address = bytes[offset + 1];
    else if (['abs', 'abx', 'aby'].includes(mode)) address = bytes.readUInt16LE(offset + 1);
    if (address < 0 || (address >= 0x2000 && address < 0x7b00 && address < 0x6000 && address >= 0x4000)) continue;
    if (address < 0x100) { zeroPage.set(address, (zeroPage.get(address) ?? 0) + 1); continue; }
    if (address >= 0x2000 && address < 0x5800) continue;
    if (address >= 0x7b00 && address < 0xff00 && !(address >= 0x7b00 && address < 0x8000)) continue;
    const page = address >> 8;
    const entry = pages.get(page) ?? { addresses: new Set(), count: 0 };
    entry.addresses.add(address);
    entry.count += 1;
    pages.set(page, entry);
  }
  console.log(`零页：${zeroPage.size} 个单元被访问，共 ${[...zeroPage.values()].reduce((sum, value) => sum + value, 0)} 次`);
  for (const [page, entry] of [...pages].sort((left, right) => left[0] - right[0])) console.log(`$${hex2(page)}xx：${entry.addresses.size} 个不同地址，${entry.count} 次`);
}
