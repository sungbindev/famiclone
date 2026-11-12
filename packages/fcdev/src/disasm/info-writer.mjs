// ------------------------------------------------------------------------
// 名称：info-writer.mjs
// 说明：把分析结果写成 da65 的信息文件：把整个映像划分成代码、数据与向量表几段，并给入口与用户符号起名字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：信息文件语法见 da65 的文档；RANGE 的结束地址包含在内；各段首尾相接、互不重叠，没有遗漏，da65 才不会把没说明的字节按默认当成代码。
// ------------------------------------------------------------------------
import { FcdevError } from '../shared/index.mjs';
import { KIND_UNKNOWN } from './analysis.mjs';

/** ca65 标识符 */
const IDENTIFIER = /^[A-Za-z_][A-Za-z0-9_]*$/;

/** 中断向量表的字节数：NMI、复位、IRQ 各一个字 */
export const VECTOR_TABLE_SIZE = 6;

/**
 * @typedef {object} Range
 * @property {number} start 起始地址
 * @property {number} end 结束地址（包含）
 * @property {'Code'|'ByteTable'|'AddrTable'} type 类型
 */

/**
 * 把映像划分成连续的段。
 * @param {{kinds: Uint8Array, base: number, hasVectors: boolean}} input 每个字节的分类、起始地址与是否有向量表（占最后 6 字节）
 * @returns {Range[]} 覆盖整个映像的段，按地址排序
 */
export function buildRanges({ kinds, base, hasVectors }) {
  const body = hasVectors ? kinds.length - VECTOR_TABLE_SIZE : kinds.length;
  const ranges = [];
  for (let offset = 0; offset < body; offset += 1) {
    const type = kinds[offset] === KIND_UNKNOWN ? 'ByteTable' : 'Code';
    const last = ranges.at(-1);
    if (last && last.type === type) last.end = base + offset;
    else ranges.push({ start: base + offset, end: base + offset, type });
  }
  if (hasVectors) ranges.push({ start: base + body, end: base + kinds.length - 1, type: 'AddrTable' });
  return ranges;
}

/**
 * 格式化十六进制地址。
 * @param {number} address 地址
 * @returns {string} 如 $C000
 */
function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 生成信息文件的内容。
 * @param {{inputName: string, outputName: string, base: number, cpu?: string, ranges: Range[], labels: {name: string, address: number}[], includes?: string[]}} input 输入输出文件名、起始地址、CPU、各段、命名的地址与符号定义文件
 * @returns {string} 信息文件内容
 * @throws {FcdevError} 符号名不是合法的标识符
 */
export function renderInfo({ inputName, outputName, base, cpu = '6502', ranges, labels, includes = [] }) {
  const lines = [
    `GLOBAL { INPUTNAME "${inputName}"; OUTPUTNAME "${outputName}"; STARTADDR ${hex(base)}; CPU "${cpu}"; PAGELENGTH 0; };`,
    ...includes.map((file) => `ASMINC { FILE "${file.replaceAll('\\', '/')}"; };`),
  ];
  const seen = new Set();
  for (const { name, address } of labels) {
    if (!IDENTIFIER.test(name)) throw new FcdevError(`符号名“${name}”不是合法的标识符`, { code: 'DISASM_SYMBOL_INVALID' });
    if (seen.has(address)) continue;
    seen.add(address);
    lines.push(`LABEL { NAME "${name}"; ADDR ${hex(address)}; };`);
  }
  for (const range of ranges) lines.push(`RANGE { START ${hex(range.start)}; END ${hex(range.end)}; TYPE ${range.type}; };`);
  return `${lines.join('\n')}\n`;
}
