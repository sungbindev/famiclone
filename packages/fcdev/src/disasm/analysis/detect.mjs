// ------------------------------------------------------------------------
// 名称：detect.mjs
// 说明：在还没有标注的字节里识别候选：填充、地址表、字符串、图形数据；只返回候选与依据，不修改任何东西。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只看既没被追到、也没有区域标注的字节；顺序是填充、地址表、字符串，前面识别出的范围不再参与后面的识别。
//       地址表：连续的 16 位值指向已识别的指令开头（值加 1 的是 RTS 跳转表）；字符串：可见 ASCII，带 $00 或最高位结尾更可信；
//       图形：只在调用者给出“图形数据端口”（把数据写进显存的寄存器地址，由目标描述提供）时才识别，依据是
//       LDA 表,X/Y 之后不远处把值写进这个端口，没有端口就不猜，因为字节本身看不出是不是图形。
// ------------------------------------------------------------------------
import { decode } from '../cpu/index.mjs';
import { addressToOffset, regionAt } from '../model/index.mjs';
import { createAddressSpace } from './address-space.mjs';
import { BYTE_KIND } from './kinds.mjs';

/** 默认参数 */
const DEFAULTS = { minPadding: 16, minString: 4, minTableEntries: 3, graphicsPorts: [] };

/** 图形识别：加载与写端口之间最多隔几条顺序指令 */
const GRAPHICS_WINDOW = 6;

/**
 * @typedef {object} Candidate
 * @property {'padding'|'words'|'string'|'graphics'} type 候选的区域类型
 * @property {number} segmentId 所在的段
 * @property {number} offset 文件偏移
 * @property {number} address CPU 地址
 * @property {number} length 字节数
 * @property {Record<string, unknown>} params 建议的区域参数
 * @property {string} evidence 依据
 * @property {'high'|'medium'|'low'} confidence 可信程度
 */

/**
 * @typedef {object} DetectOptions
 * @property {number} [minPadding] 填充的最少字节数，默认 16
 * @property {number} [minString] 字符串的最少字符数，默认 4
 * @property {number} [minTableEntries] 地址表里指向指令开头的表项最少个数，默认 3
 * @property {number[]} [graphicsPorts] 图形数据端口的 CPU 地址，默认没有
 */

/**
 * 十六进制写出地址。
 * @param {number} address 地址
 * @returns {string} 如 $C000
 */
function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 识别候选。
 * @param {{bytes: Uint8Array, layout: import('../model/layout.mjs').Layout, analysis: import('./tracer.mjs').Analysis, regions: import('../model/regions.mjs').RegionMap, options?: DetectOptions}} input 映像字节、布局、追踪结果、区域表与选项
 * @returns {{padding: Candidate[], tables: Candidate[], strings: Candidate[], graphics: Candidate[]}} 各类候选，按偏移排序
 */
export function detectCandidates({ bytes, layout, analysis, regions, options = {} }) {
  const settings = { ...DEFAULTS, ...options };
  const space = createAddressSpace(layout);
  const result = { padding: [], tables: [], strings: [], graphics: [] };

  const isFree = (offset) => analysis.kinds[offset] === BYTE_KIND.UNKNOWN && regionAt(regions, offset) === null;
  const candidate = (segment, offset, length, type, params, evidence, confidence) => ({
    type, segmentId: segment.id, offset, address: segment.address + (offset - segment.fileOffset), length, params, evidence, confidence,
  });

  /** 把范围 [start, end) 里被 taken 占掉的部分去掉，返回剩下的连续子范围 */
  const subtract = (start, end, taken) => {
    const pieces = [];
    let cursor = start;
    for (const span of taken.filter((item) => item.end > start && item.start < end).sort((a, b) => a.start - b.start)) {
      if (span.start > cursor) pieces.push({ start: cursor, end: span.start });
      cursor = Math.max(cursor, span.end);
    }
    if (cursor < end) pieces.push({ start: cursor, end });
    return pieces;
  };

  for (const segment of layout.segments) {
    const segmentEnd = segment.fileOffset + segment.length;
    // 连续的空闲字节
    const runs = [];
    let runStart = -1;
    for (let offset = segment.fileOffset; offset <= segmentEnd; offset += 1) {
      const free = offset < segmentEnd && isFree(offset);
      if (free && runStart < 0) runStart = offset;
      else if (!free && runStart >= 0) {
        runs.push({ start: runStart, end: offset });
        runStart = -1;
      }
    }

    const taken = [];
    // 填充：同一个字节重复
    for (const run of runs) {
      let start = run.start;
      while (start < run.end) {
        let end = start + 1;
        while (end < run.end && bytes[end] === bytes[start]) end += 1;
        if (end - start >= settings.minPadding) {
          result.padding.push(candidate(segment, start, end - start, 'padding', { value: bytes[start] }, `${end - start} 个重复的 $${bytes[start].toString(16).toUpperCase().padStart(2, '0')}`, 'high'));
          taken.push({ start, end });
        }
        start = end;
      }
    }

    // 地址表：连续的 16 位值指向已识别的指令开头
    for (const run of runs) {
      for (const piece of subtract(run.start, run.end, taken)) {
        let start = piece.start;
        while (start + 1 < piece.end) {
          let best = null;
          for (const adjust of [0, 1]) {
            let entries = 0;
            let strong = 0;
            let lastStrong = 0;
            for (let offset = start; offset + 1 < piece.end; offset += 2) {
              const target = ((bytes[offset] | (bytes[offset + 1] << 8)) + adjust) & 0xffff;
              const resolved = space.resolve(target, segment.id);
              if (resolved.status !== 'ok') break;
              entries += 1;
              if (analysis.kinds[addressToOffset(resolved.segment, target)] === BYTE_KIND.OPCODE) {
                strong += 1;
                lastStrong = entries;
              }
            }
            if (strong >= settings.minTableEntries && (best === null || strong > best.strong)) best = { adjust, strong, entries: lastStrong };
          }
          if (best === null) {
            start += 1;
            continue;
          }
          const length = best.entries * 2;
          result.tables.push(candidate(segment, start, length, 'words', { adjust: best.adjust },
            `${best.entries} 个 16 位值${best.adjust === 1 ? '（加 1 后）' : ''}，其中 ${best.strong} 个指向已识别的指令开头${best.adjust === 1 ? '，疑似 RTS 跳转表' : ''}`,
            best.strong === best.entries ? 'high' : 'medium'));
          taken.push({ start, end: start + length });
          start += length;
        }
      }
    }

    // 字符串：可见 ASCII，以 $00 或最高位置位的字符结尾更可信
    const printable = (value) => value >= 0x20 && value <= 0x7e;
    for (const run of runs) {
      for (const piece of subtract(run.start, run.end, taken)) {
        let start = piece.start;
        while (start < piece.end) {
          if (!printable(bytes[start])) {
            start += 1;
            continue;
          }
          let end = start;
          while (end < piece.end && printable(bytes[end])) end += 1;
          let length = end - start;
          let terminator = null;
          if (end < piece.end && bytes[end] === 0x00) {
            terminator = 0;
            length += 1;
          } else if (end < piece.end && bytes[end] >= 0x80 && printable(bytes[end] & 0x7f)) {
            terminator = 'high-bit';
            length += 1;
          }
          const characters = end - start + (terminator === 'high-bit' ? 1 : 0);
          if (characters >= settings.minString) {
            const text = Buffer.from(bytes.subarray(start, end)).toString('latin1') + (terminator === 'high-bit' ? String.fromCharCode(bytes[end] & 0x7f) : '');
            const preview = text.length > 32 ? `${text.slice(0, 32)}…` : text;
            const confidence = terminator !== null ? 'high' : characters >= 8 ? 'medium' : 'low';
            result.strings.push(candidate(segment, start, length, 'string', { encoding: 'ascii', terminator }, `${characters} 个字符${terminator === null ? '，没有结尾标记' : terminator === 0 ? '，以 $00 结尾' : '，最后一个字符的最高位置位'}：“${preview}”`, confidence));
          }
          start += Math.max(length, 1);
        }
      }
    }

    // 图形：LDA 表,X/Y 之后不远处写进图形数据端口
    if (settings.graphicsPorts.length > 0) {
      const seen = new Set();
      for (const [offset, info] of analysis.instructions) {
        if (info.segmentId !== segment.id) continue;
        const load = decode(bytes, offset, info.address);
        if (load.mnemonic !== 'lda' || (load.mode !== 'absoluteX' && load.mode !== 'absoluteY')) continue;
        const resolved = space.resolve(load.operand, segment.id);
        if (resolved.status !== 'ok') continue;
        const dataOffset = addressToOffset(resolved.segment, load.operand);
        if (seen.has(dataOffset) || !isFree(dataOffset)) continue;
        let next = offset + info.size;
        for (let step = 0; step < GRAPHICS_WINDOW && analysis.instructions.has(next); step += 1) {
          const following = decode(bytes, next, analysis.instructions.get(next).address);
          if (following.mnemonic === 'sta' && following.mode === 'absolute' && settings.graphicsPorts.includes(following.operand)) {
            let end = dataOffset;
            const resolvedEnd = resolved.segment.fileOffset + resolved.segment.length;
            while (end < resolvedEnd && isFree(end)) end += 1;
            seen.add(dataOffset);
            result.graphics.push(candidate(resolved.segment, dataOffset, end - dataOffset, 'graphics', { format: 'unspecified' },
              `${hex(info.address)} 的 ${load.mnemonic} 从 ${hex(load.operand)} 取数，随后写进图形数据端口 ${hex(following.operand)}；长度取到下一个已标注的字节，实际大小要看循环次数`, 'medium'));
            break;
          }
          if (following.flow !== 'next') break;
          next += analysis.instructions.get(next).size;
        }
      }
    }
  }
  for (const list of Object.values(result)) list.sort((a, b) => a.offset - b.offset);
  return result;
}
