// ------------------------------------------------------------------------
// 名称：find.mjs
// 说明：在映像里搜索：字节序列（可带通配符）、文本（ASCII，或每个字符最高位为 1）、指令序列（助记符加操作数模式）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只搜布局里的段；匹配不跨段。指令搜索只看按区域解码出的指令，连续的指令才算一组；操作数模式里的 * 匹配任意文字。
//       结果分页：cursor 是上一页最后一个匹配的下一个文件偏移。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { formatHex, formatInstruction } from '../cpu/index.mjs';

/** 默认每页条数与上限 */
export const DEFAULT_LIMIT = 50;
export const MAX_LIMIT = 500;

/**
 * 抛出参数错误。
 * @param {string} message 说明
 * @returns {never}
 */
function invalid(message) {
  throw new FcdevError(message, { code: 'DISASM_PARAM_INVALID' });
}

/**
 * 解析十六进制字节模式，如 `A9 ?? 8D 00 20`，`??` 匹配任意字节。
 * @param {string} text 模式
 * @returns {Array<number|null>} 每个位置的字节，通配符为 null
 */
export function parseBytePattern(text) {
  const tokens = text.trim().split(/[\s,]+/).filter((token) => token !== '');
  if (tokens.length === 0) invalid('字节模式不能为空');
  return tokens.map((token) => {
    if (token === '??' || token === '?') return null;
    const match = /^(?:\$|0[xX])?([0-9A-Fa-f]{2})$/.exec(token);
    if (match === null) invalid(`字节模式里的“${token}”不是两位十六进制或 ??`);
    return Number.parseInt(match[1], 16);
  });
}

/**
 * 把文本换成要搜索的字节模式。
 * @param {string} text 文本，只能是 ASCII 可见字符
 * @param {'ascii'|'ascii-high'} encoding ascii 为原字节，ascii-high 为每个字符最高位置 1
 * @returns {number[]} 字节
 */
export function textToPattern(text, encoding) {
  if (text === '') invalid('要搜索的文本不能为空');
  if (encoding !== 'ascii' && encoding !== 'ascii-high') invalid('编码只能是 ascii 或 ascii-high');
  return [...text].map((character) => {
    const code = character.codePointAt(0);
    if (code < 0x20 || code > 0x7e) invalid(`文本里的“${character}”不是 ASCII 可见字符`);
    return encoding === 'ascii-high' ? code | 0x80 : code;
  });
}

/**
 * 规范化分页参数。
 * @param {{limit?: number, cursor?: string|number}} page 条数与游标
 * @returns {{limit: number, cursor: number}} 规范化后的条数与游标（起点）
 */
export function normalizePage({ limit, cursor }) {
  const count = limit ?? DEFAULT_LIMIT;
  if (!Number.isInteger(count) || count < 1 || count > MAX_LIMIT) invalid(`limit 必须是 1 到 ${MAX_LIMIT} 的整数`);
  const start = cursor === undefined ? 0 : Number(cursor);
  if (!Number.isInteger(start) || start < 0) invalid('cursor 不合法，用上一页返回的 nextCursor');
  return { limit: count, cursor: start };
}

/**
 * 搜索字节模式。
 * @param {{project: import('../model/project.mjs').Project, bytes: Uint8Array, pattern: Array<number|null>, limit?: number, cursor?: string|number}} input 工程、映像、模式与分页
 * @returns {{matches: Array<{segmentId: number, address: number, offset: number}>, nextCursor: string|null}} 匹配与下一页游标（没有更多时为 null）
 */
export function findBytes({ project, bytes, pattern, ...page }) {
  const { limit, cursor } = normalizePage(page);
  const matches = [];
  let nextCursor = null;
  for (const segment of [...project.layout.segments].sort((a, b) => a.fileOffset - b.fileOffset)) {
    const last = segment.fileOffset + segment.length - pattern.length;
    for (let offset = Math.max(segment.fileOffset, cursor); offset <= last; offset += 1) {
      if (!pattern.every((value, position) => value === null || bytes[offset + position] === value)) continue;
      if (matches.length === limit) {
        nextCursor = String(offset);
        return { matches, nextCursor };
      }
      matches.push({ segmentId: segment.id, address: segment.address + (offset - segment.fileOffset), offset });
    }
  }
  return { matches, nextCursor };
}

/**
 * 把指令搜索模式拆成每条指令一个正则。
 * @param {string} text 模式，指令之间用分号分隔，如 `lda #*; sta $2000`
 * @returns {RegExp[]} 每条指令的正则（对格式化后的指令文字做不分大小写的整行匹配）
 */
function parseInstructionPattern(text) {
  const parts = text.split(';').map((part) => part.trim()).filter((part) => part !== '');
  if (parts.length === 0) invalid('指令模式不能为空');
  return parts.map((part) => {
    const escaped = part.replace(/\s+/g, ' ').replace(/[.+?^${}()|[\]\\]/g, '\\$&').replace(/\*/g, '.*');
    return new RegExp(`^${escaped}$`, 'i');
  });
}

/**
 * 搜索指令序列。
 * @param {{scan: import('../analysis/code-scan.mjs').CodeScan, pattern: string, limit?: number, cursor?: string|number}} input 指令扫描结果、模式与分页
 * @returns {{matches: Array<{segmentId: number, address: number, offset: number, text: string[]}>, nextCursor: string|null}} 匹配与下一页游标
 */
export function findInstructions({ scan, pattern, ...page }) {
  const { limit, cursor } = normalizePage(page);
  const expressions = parseInstructionPattern(pattern);
  const texts = scan.list.map((item) => formatInstruction(item.instruction));
  const matches = [];
  for (let position = 0; position + expressions.length <= scan.list.length; position += 1) {
    const first = scan.list[position];
    if (first.offset < cursor) continue;
    let ok = true;
    for (let step = 0; step < expressions.length && ok; step += 1) {
      const item = scan.list[position + step];
      const previous = scan.list[position + step - 1];
      if (step > 0 && previous.offset + previous.size !== item.offset) ok = false;
      else if (!expressions[step].test(texts[position + step])) ok = false;
    }
    if (!ok) continue;
    if (matches.length === limit) return { matches, nextCursor: String(first.offset) };
    matches.push({
      segmentId: first.segmentId,
      address: first.address,
      offset: first.offset,
      text: expressions.map((_, step) => `${formatHex(scan.list[position + step].address, 4)}  ${texts[position + step]}`),
    });
  }
  return { matches, nextCursor: null };
}
