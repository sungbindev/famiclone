// ------------------------------------------------------------------------
// 名称：listing.mjs
// 说明：带符号与注释的反汇编清单：按地址范围显示指令、数据与符号标签，供查看代码用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：指令的操作数在地址正好有符号时写符号名，否则写数值；操作数提示不参与（它只影响生成的源码）；
//       没被追到的字节与数据区域按每行 8 个字节写成 .byte，行尾标出区域类型。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { scanCode } from '../analysis/index.mjs';
import { formatHex, formatInstruction } from '../cpu/index.mjs';
import { addressToOffset, getSegment, regionAt } from '../model/index.mjs';

/** 数据每行的字节数 */
const DATA_ROW = 8;

/** 默认的最大行数 */
export const DEFAULT_LISTING_LINES = 200;

/**
 * 把字节写成空格分隔的十六进制。
 * @param {Uint8Array} bytes 字节
 * @param {number} start 起点
 * @param {number} length 字节数
 * @returns {string} 如 A9 05
 */
function hexBytes(bytes, start, length) {
  return [...bytes.subarray(start, start + length)].map((value) => value.toString(16).toUpperCase().padStart(2, '0')).join(' ');
}

/**
 * @typedef {object} Listing
 * @property {string} text 清单文字
 * @property {number} segmentId 段号
 * @property {number} start 起始 CPU 地址
 * @property {number} end 实际结束的 CPU 地址（不含）
 * @property {boolean} truncated 因为行数上限没有显示完
 * @property {number|null} nextAddress 截断时下一行的 CPU 地址
 */

/**
 * 生成清单。
 * @param {{project: import('../model/project.mjs').Project, bytes: Uint8Array, index: import('./symbol-index.mjs').SymbolIndex, segmentId: number, address?: number, length?: number, maxLines?: number, scan?: import('../analysis/code-scan.mjs').CodeScan}} input
 *   工程、映像、符号索引、段号、起始地址（默认段首）、字节数（默认 256）、最大行数、指令扫描结果（不给时现扫）
 * @returns {Listing} 清单
 * @throws {FcdevError} 段不存在，或起点不在段内
 */
export function renderListing({ project, bytes, index, segmentId, address, length = 256, maxLines = DEFAULT_LISTING_LINES, scan = scanCode(project, bytes) }) {
  const segment = getSegment(project.layout, segmentId);
  if (segment === undefined) throw new FcdevError(`没有段 ${segmentId}`, { code: 'DISASM_RANGE_INVALID' });
  const segmentEnd = segment.address + segment.length;
  const start = address ?? segment.address;
  if (!Number.isInteger(start) || start < segment.address || start >= segmentEnd) {
    throw new FcdevError(`起始地址 ${formatHex(start, 4)} 不在段 ${segmentId}（${formatHex(segment.address, 4)}-${formatHex(segmentEnd - 1, 4)}）里`, { code: 'DISASM_RANGE_INVALID' });
  }
  const end = Math.min(segmentEnd, start + Math.max(1, length));
  const blockComments = new Map();
  const inlineComments = new Map();
  for (const comment of project.comments) {
    if (comment.segmentId !== segmentId) continue;
    const target = comment.kind === 'block' ? blockComments : inlineComments;
    target.set(comment.address, comment.text);
  }

  const lines = [`; 段 ${segment.id}${segment.bank === null ? '' : `，bank ${segment.bank}`}，${formatHex(start, 4)}-${formatHex(end - 1, 4)}`];
  const limit = maxLines + 1;
  let current = start;
  let nextAddress = null;
  while (current < end) {
    if (lines.length >= limit) {
      nextAddress = current;
      break;
    }
    const offset = addressToOffset(segment, current);
    const symbol = index.at(segmentId, current);
    const block = blockComments.get(current);
    if (block !== undefined) for (const text of block.split('\n')) lines.push(`; ${text}`);
    if (symbol !== null) lines.push(`${symbol.name}:${symbol.description === '' ? '' : `  ; ${symbol.description.replace(/\n/g, ' ')}`}`);
    const inline = inlineComments.get(current);
    const instruction = scan.byOffset.get(offset);
    if (instruction !== undefined && current + instruction.size <= segmentEnd) {
      const decoded = instruction.instruction;
      const targetAddress = decoded.target ?? decoded.reference;
      const name = targetAddress === null ? null : index.nameFor(targetAddress, segmentId);
      const text = formatInstruction(decoded, name === null ? {} : { expression: name });
      lines.push(`${formatHex(current, 4).slice(1)}  ${hexBytes(bytes, offset, instruction.size).padEnd(8)}  ${text}${inline === undefined ? '' : `  ; ${inline}`}`);
      current += instruction.size;
      continue;
    }
    // 数据：到下一个符号、注释、指令开头、区域边界或行满为止
    const region = regionAt(project.regions, offset);
    const regionEnd = region === null ? Number.POSITIVE_INFINITY : region.offset + region.length;
    let count = 1;
    while (count < DATA_ROW && current + count < end) {
      const nextOffset = offset + count;
      const nextAddressHere = current + count;
      if (nextOffset >= regionEnd || scan.byOffset.has(nextOffset) || index.at(segmentId, nextAddressHere) !== null || blockComments.has(nextAddressHere) || inlineComments.has(nextAddressHere)) break;
      count += 1;
    }
    const values = [...bytes.subarray(offset, offset + count)];
    const printable = values.every((value) => value >= 0x20 && value <= 0x7e) ? `  "${String.fromCharCode(...values)}"` : '';
    lines.push(`${formatHex(current, 4).slice(1)}  ${hexBytes(bytes, offset, count).padEnd(DATA_ROW * 3 - 1)}  .byte ${values.map((value) => formatHex(value, 2)).join(',')}  ; ${region === null ? 'unknown' : region.type}${printable}${inline === undefined ? '' : `  ; ${inline}`}`);
    current += count;
  }
  return { text: lines.join('\n'), segmentId, start, end: nextAddress ?? end, truncated: nextAddress !== null, nextAddress };
}
