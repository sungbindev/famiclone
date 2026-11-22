// ------------------------------------------------------------------------
// 名称：detect.ts
// 说明：在还没有标注的字节里识别候选：填充、地址表、字符串、图形数据；只返回候选与依据，不修改任何
//       东西。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只看既没被追到、也没有区域标注的字节；顺序是填充、地址表、字符串，前面识别出的范围不再参与
//       后面的识别。地址表：连续的 16 位值指向已识别的指令开头（值加 1 的是 RTS 跳转表）；字符串：
//       可见 ASCII，带 $00 或最高位结尾更可信；图形：只在调用者给出“图形数据端口”（把数据写进显存
//       的寄存器地址，由目标描述提供）时才识别，依据是LDA 表,X/Y 之后不远处把值写进这个端口，没有端
//       口就不猜，因为字节本身看不出是不是图形。
// ------------------------------------------------------------------------
import { decode } from '../cpu/index';
import { addressToOffset, regionAt } from '../model/index';
import { createAddressSpace } from './address-space';
import { BYTE_KIND } from './kinds';
import type { Layout, Segment } from '../model/layout';
import type { Analysis } from './tracer';
import type { RegionMap } from '../model/regions';

/** 默认参数 */
const DEFAULTS: Required<DetectOptions> = { minPadding: 16, minString: 4, minTableEntries: 3, graphicsPorts: [] };

/** 文件内的一段偏移范围：[start, end) */
interface Span {
  start: number;
  end: number;
}

/** 图形识别：加载与写端口之间最多隔几条顺序指令 */
const GRAPHICS_WINDOW = 6;

export interface Candidate {
  /** 候选的区域类型 */
  type: 'padding'|'words'|'string'|'graphics';
  /** 所在的段 */
  segmentId: number;
  /** 文件偏移 */
  offset: number;
  /** CPU 地址 */
  address: number;
  /** 字节数 */
  length: number;
  /** 建议的区域参数 */
  params: Record<string, unknown>;
  /** 依据 */
  evidence: string;
  /** 可信程度 */
  confidence: 'high'|'medium'|'low';
}

export interface DetectOptions {
  /** 填充的最少字节数，默认 16 */
  minPadding?: number;
  /** 字符串的最少字符数，默认 4 */
  minString?: number;
  /** 地址表里指向指令开头的表项最少个数，默认 3 */
  minTableEntries?: number;
  /** 图形数据端口的 CPU 地址，默认没有 */
  graphicsPorts?: number[];
}

/**
 * 十六进制写出地址。
 * @param address 地址
 * @returns 如 $C000
 */
function hex(address: number): string {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 把范围 [start, end) 里被 taken 占掉的部分去掉，返回剩下的连续子范围。
 * @param start 起点
 * @param end 终点（不含）
 * @param taken 已经被前面识别占用的范围
 * @returns 剩下的连续子范围
 */
function subtract(start: number, end: number, taken: Span[]): Span[] {
  const pieces: Span[] = [];
  let cursor = start;
  for (const span of taken.filter((item) => item.end > start && item.start < end).toSorted((a, b) => a.start - b.start)) {
    if (span.start > cursor) pieces.push({ start: cursor, end: span.start });
    cursor = Math.max(cursor, span.end);
  }
  if (cursor < end) pieces.push({ start: cursor, end });
  return pieces;
}

/**
 * 字节是不是可见 ASCII。
 * @param value 字节
 * @returns 可见时为 true
 */
function printable(value: number | undefined): boolean {
  return value !== undefined && value >= 0x20 && value <= 0x7e;
}

/**
 * 识别候选。
 * @param input 映像字节、布局、追踪结果、区域表与选项
 * @returns 各类候选，按偏移排序
 */
export function detectCandidates({ bytes, layout, analysis, regions, options = {} }: {bytes: Uint8Array, layout: Layout, analysis: Analysis, regions: RegionMap, options?: DetectOptions}): {padding: Candidate[], tables: Candidate[], strings: Candidate[], graphics: Candidate[]} {
  const settings = { ...DEFAULTS, ...options };
  const space = createAddressSpace(layout);
  const result: {padding: Candidate[], tables: Candidate[], strings: Candidate[], graphics: Candidate[]} = { padding: [], tables: [], strings: [], graphics: [] };

  const isFree = (offset: number) => analysis.kinds[offset] === BYTE_KIND.UNKNOWN && regionAt(regions, offset) === null;
  const candidate = (segment: Readonly<Segment>, offset: number, length: number, type: Candidate['type'], params: Record<string, unknown>, evidence: string, confidence: Candidate['confidence']): Candidate => ({
    type, segmentId: segment.id, offset, address: segment.address + (offset - segment.fileOffset), length, params, evidence, confidence,
  });

  for (const segment of layout.segments) {
    const segmentEnd = segment.fileOffset + segment.length;
    // 连续的空闲字节
    const runs: Span[] = [];
    let runStart = -1;
    for (let offset = segment.fileOffset; offset <= segmentEnd; offset += 1) {
      const free = offset < segmentEnd && isFree(offset);
      if (free && runStart < 0) runStart = offset;
      else if (!free && runStart >= 0) {
        runs.push({ start: runStart, end: offset });
        runStart = -1;
      }
    }

    const taken: Span[] = [];
    // 填充：同一个字节重复
    for (const run of runs) {
      let start = run.start;
      while (start < run.end) {
        const fill = bytes[start] ?? 0;
        let end = start + 1;
        while (end < run.end && bytes[end] === fill) end += 1;
        if (end - start >= settings.minPadding) {
          result.padding.push(candidate(segment, start, end - start, 'padding', { value: fill }, `${end - start} 个重复的 $${fill.toString(16).toUpperCase().padStart(2, '0')}`, 'high'));
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
          let best: {adjust: number, strong: number, entries: number} | null = null;
          for (const adjust of [0, 1]) {
            let entries = 0;
            let strong = 0;
            let lastStrong = 0;
            for (let offset = start; offset + 1 < piece.end; offset += 2) {
              const target = (((bytes[offset] ?? 0) | ((bytes[offset + 1] ?? 0) << 8)) + adjust) & 0xffff;
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
          let terminator: 0 | 'high-bit' | null = null;
          const followByte = end < piece.end ? (bytes[end] ?? 0) : null;
          if (followByte === 0x00) {
            terminator = 0;
            length += 1;
          } else if (followByte !== null && followByte >= 0x80 && printable(followByte & 0x7f)) {
            terminator = 'high-bit';
            length += 1;
          }
          const characters = end - start + (terminator === 'high-bit' ? 1 : 0);
          if (characters >= settings.minString) {
            const text = Buffer.from(bytes.subarray(start, end)).toString('latin1') + (terminator === 'high-bit' ? String.fromCharCode((followByte ?? 0) & 0x7f) : '');
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
      const seen = new Set<number>();
      for (const [offset, info] of analysis.instructions) {
        if (info.segmentId !== segment.id) continue;
        const load = decode(bytes, offset, info.address);
        if (!load.ok || load.mnemonic !== 'lda' || (load.mode !== 'absoluteX' && load.mode !== 'absoluteY')) continue;
        const resolved = space.resolve(load.operand!, segment.id);
        if (resolved.status !== 'ok') continue;
        const dataOffset = addressToOffset(resolved.segment, load.operand!);
        if (seen.has(dataOffset) || !isFree(dataOffset)) continue;
        let next = offset + info.size;
        for (let step = 0; step < GRAPHICS_WINDOW && analysis.instructions.has(next); step += 1) {
          const following = decode(bytes, next, analysis.instructions.get(next)!.address);
          if (following.ok && following.mnemonic === 'sta' && following.mode === 'absolute' && settings.graphicsPorts.includes(following.operand!)) {
            let end = dataOffset;
            const resolvedEnd = resolved.segment.fileOffset + resolved.segment.length;
            while (end < resolvedEnd && isFree(end)) end += 1;
            seen.add(dataOffset);
            result.graphics.push(candidate(resolved.segment, dataOffset, end - dataOffset, 'graphics', { format: 'unspecified' },
              `${hex(info.address)} 的 ${load.mnemonic} 从 ${hex(load.operand!)} 取数，随后写进图形数据端口 ${hex(following.operand!)}；长度取到下一个已标注的字节，实际大小要看循环次数`, 'medium'));
            break;
          }
          if (!following.ok || following.flow !== 'next') break;
          next += analysis.instructions.get(next)!.size;
        }
      }
    }
  }
  for (const list of Object.values(result)) list.sort((a, b) => a.offset - b.offset);
  return result;
}
