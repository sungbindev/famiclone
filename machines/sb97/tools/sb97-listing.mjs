// ------------------------------------------------------------------------
// 名称：sb97-listing.mjs
// 说明：把一次静态分析的结果写成汇编清单：能确定是代码的按指令写，其余按字节写。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：清单只是阅读用的，不保证能重新汇编出同一份 ROM；标签只给被跳转或调用过的地址，名字取入口名，没有则用 L_地址。
// ------------------------------------------------------------------------
import { formatInstruction } from './disasm.mjs';
import { KIND_OPCODE } from './sb97-analysis.mjs';
import { WINDOW_START } from './sb97-memory.mjs';

const hex4 = (value) => value.toString(16).toUpperCase().padStart(4, '0');
const hex2 = (value) => value.toString(16).toUpperCase().padStart(2, '0');

/** 数据一行最多放几个字节 */
const BYTES_PER_ROW = 16;

/**
 * 一个地址的标签名，没有被引用过则返回 null。
 * @param {import('./sb97-analysis.mjs').Analysis} analysis 分析结果
 * @param {number} address 地址
 * @returns {string|null}
 */
export function labelOf(analysis, address) {
  if (!analysis.references.has(address)) return null;
  return analysis.names.get(address) ?? `L_${hex4(address)}`;
}

/**
 * 生成 [first, last] 这一段地址的清单。
 * @param {import('./sb97-analysis.mjs').Analysis} analysis 分析结果
 * @param {{title: string, first: number, last: number}} options 标题与地址范围（含两端）
 * @returns {string}
 */
export function renderListing(analysis, { title, first, last }) {
  const { view, kind, instructions } = analysis;
  const lines = [`; ${title}`, `; 地址 $${hex4(first)}-$${hex4(last)}，视图 ${view.name}`, ''];
  let address = first;
  while (address <= last) {
    const label = labelOf(analysis, address);
    if (label !== null) lines.push(`${label}:`);
    if (kind[address - WINDOW_START] === KIND_OPCODE) {
      const ins = instructions.get(address);
      const bytes = [];
      for (let i = 0; i < ins.length; i += 1) bytes.push(hex2(view.read(address + i)));
      lines.push(`${hex4(address)}  ${bytes.join(' ').padEnd(8)}  ${formatInstruction(ins, (target) => labelOf(analysis, target))}`);
      address += ins.length;
      continue;
    }
    const tableTarget = analysis.tableWords.get(address);
    if (tableTarget !== undefined) {
      lines.push(`${hex4(address)}  .word ${labelOf(analysis, tableTarget) ?? `$${hex4(tableTarget)}`}`);
      address += 2;
      continue;
    }
    let end = address + 1;
    while (end <= last && end - address < BYTES_PER_ROW && kind[end - WINDOW_START] !== KIND_OPCODE && !analysis.references.has(end) && !analysis.tableWords.has(end)) end += 1;
    const bytes = [];
    for (let i = address; i < end; i += 1) bytes.push(`$${hex2(view.read(i))}`);
    lines.push(`${hex4(address)}  .byte ${bytes.join(',')}`);
    address = end;
  }
  lines.push('');
  return lines.join('\n');
}
