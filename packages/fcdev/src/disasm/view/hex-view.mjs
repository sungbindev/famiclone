// ------------------------------------------------------------------------
// 名称：hex-view.mjs
// 说明：十六进制与字符视图：按 CPU 地址对齐，一行 16 个字节（可调），可带符号标注。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：行按 CPU 地址对齐（如 $C000、$C010），范围的头尾不满一行时用空位补齐；字符视图只显示 ASCII 可见字符，其余为 .。
//       区域类型标注依赖阶段 3 的区域模型，到时再加。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { addressToOffset, getSegment } from '../model/index.mjs';

/** 每行字节数的上限 */
const MAX_BYTES_PER_ROW = 64;

/**
 * @typedef {object} HexViewOptions
 * @property {Uint8Array} bytes 映像文件的全部字节
 * @property {import('../model/layout.mjs').Layout} layout 布局
 * @property {number} segmentId 要看的段号
 * @property {number} [address] 起始 CPU 地址，默认是段的第一个字节
 * @property {number} [length] 字节数，默认到段尾；超出段尾的部分被截掉
 * @property {number} [bytesPerRow] 每行字节数，默认 16
 * @property {ReadonlyMap<number, string>} [labels] CPU 地址到符号名；落在某一行里的符号写在那一行末尾
 */

/**
 * @typedef {object} HexRow
 * @property {number} address 这一行第一列的 CPU 地址（对齐后，可能早于范围的起点）
 * @property {Array<number|null>} bytes 每一列的字节；不在范围内的列为 null
 * @property {string} text 字符视图，不在范围内的列是空格
 * @property {string[]} labels 这一行里符号的文字，如 `$C004 reset`
 */

/**
 * 抛出范围错误。
 * @param {string} message 说明
 * @returns {never}
 */
function invalid(message) {
  throw new FcdevError(message, { code: 'DISASM_RANGE_INVALID' });
}

/**
 * 把 CPU 地址写成十六进制。
 * @param {number} value 地址
 * @returns {string} 四位大写十六进制
 */
function hex4(value) {
  return value.toString(16).toUpperCase().padStart(4, '0');
}

/**
 * 生成十六进制视图的行。
 * @param {HexViewOptions} options 选项
 * @returns {{segment: import('../model/layout.mjs').Segment, start: number, end: number, rows: HexRow[]}} 段、实际的起止地址（end 不含）与行
 * @throws {FcdevError} 段不存在，或起点不在段内，或参数不合法
 */
export function buildHexRows({ bytes, layout, segmentId, address, length, bytesPerRow = 16, labels = new Map() }) {
  const segment = getSegment(layout, segmentId);
  if (segment === undefined) invalid(`没有段 ${segmentId}`);
  if (!Number.isInteger(bytesPerRow) || bytesPerRow < 1 || bytesPerRow > MAX_BYTES_PER_ROW) invalid(`每行字节数必须是 1 到 ${MAX_BYTES_PER_ROW} 的整数`);
  const segmentEnd = segment.address + segment.length;
  const start = address ?? segment.address;
  if (!Number.isInteger(start) || start < segment.address || start >= segmentEnd) invalid(`起始地址 $${hex4(start)} 不在段 ${segmentId}（$${hex4(segment.address)}-$${hex4(segmentEnd - 1)}）里`);
  if (length !== undefined && (!Number.isInteger(length) || length < 1)) invalid('字节数必须是正整数');
  const end = Math.min(segmentEnd, length === undefined ? segmentEnd : start + length);

  const rows = [];
  for (let base = start - (start % bytesPerRow); base < end; base += bytesPerRow) {
    const cells = [];
    let text = '';
    const rowLabels = [];
    for (let column = 0; column < bytesPerRow; column += 1) {
      const current = base + column;
      if (current < start || current >= end) {
        cells.push(null);
        text += ' ';
        continue;
      }
      const value = bytes[addressToOffset(segment, current)];
      cells.push(value);
      text += value >= 0x20 && value <= 0x7e ? String.fromCharCode(value) : '.';
      const name = labels.get(current);
      if (name !== undefined) rowLabels.push(`$${hex4(current)} ${name}`);
    }
    rows.push({ address: base, bytes: cells, text, labels: rowLabels });
  }
  return { segment, start, end, rows };
}

/**
 * 渲染十六进制视图：第一行是段的说明，之后每行一个地址、各列字节、字符视图和符号。
 * @param {HexViewOptions} options 选项
 * @returns {string} 多行文字
 * @throws {FcdevError} 同 buildHexRows
 */
export function renderHexView(options) {
  const { segment, start, end, rows } = buildHexRows(options);
  const bank = segment.bank === null ? '' : `，bank ${segment.bank}`;
  const lines = [`; 段 ${segment.id}${bank}，$${hex4(start)}-$${hex4(end - 1)}，文件偏移 $${addressToOffset(segment, start).toString(16).toUpperCase()}`];
  for (const row of rows) {
    const cells = row.bytes.map((value, column) => `${column > 0 && column % 8 === 0 ? ' ' : ''}${value === null ? '  ' : value.toString(16).toUpperCase().padStart(2, '0')}`);
    const suffix = row.labels.length > 0 ? `  ; ${row.labels.join('，')}` : '';
    lines.push(`${hex4(row.address)}  ${cells.join(' ')}  |${row.text}|${suffix}`);
  }
  return lines.join('\n');
}
