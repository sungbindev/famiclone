// ------------------------------------------------------------------------
// 名称：xrefs.ts
// 说明：交叉引用：找出每个地址被哪些指令或地址表引用，区分调用、跳转、分支、读、写、先读后写、取地
//       址、地址表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：取地址指两条立即数加载组成的 16 位地址（如 LDA #<标签 与 LDX #>标签），按“低字节在前或高字
//       节在前”各试一次，结果落在某个段里才记录，并标 likely（是推测，不像绝对地址那样确定）。间接
//       JMP 算对指针的读。
// ------------------------------------------------------------------------
import { decode, memoryAccess } from '../cpu/index';
import { expectSegment } from '../model/index';
import { createAddressSpace } from './address-space';
import type { DecodedInstruction } from '../cpu/decode';
import type { Layout } from '../model/layout';
import type { Analysis } from './tracer';

/** 取地址的两条立即数加载之间最多隔几条顺序指令 */
const ADDRESS_PAIR_WINDOW = 4;

/** 带变址的寻址方式 */
const INDEXED_MODES = new Set(['zeroPageX', 'zeroPageY', 'absoluteX', 'absoluteY', 'indexedIndirect', 'indirectIndexed']);

/** 立即数加载指令 */
const IMMEDIATE_LOADS = new Set(['lda', 'ldx', 'ldy']);

export interface Xref {
  /** 引用的种类 */
  kind: 'call'|'jump'|'branch'|'read'|'write'|'readWrite'|'address'|'table';
  /** 引用处所在的段 */
  segmentId: number;
  /** 引用处的 CPU 地址（指令地址，或地址表项的地址） */
  address: number;
  /** 被引用的地址所在的段；无法确定或不在任何段里时为 null */
  targetSegmentId: number|null;
  /** 指令的助记符 */
  mnemonic?: string;
  /** 是否带变址（被引用的是表的起点） */
  indexed?: boolean;
  /** 是推测的引用 */
  likely?: boolean;
}

export interface XrefIndex {
  /** 被引用的 CPU 地址到引用 */
  byTarget: Map<number, Xref[]>;
  /** 引用总数 */
  count: number;
}

/**
 * 建立交叉引用索引。
 * @param input 映像字节、布局与追踪结果
 * @returns 索引
 */
export function buildXrefs({ bytes, layout, analysis }: {bytes: Uint8Array, layout: Layout, analysis: Analysis}): XrefIndex {
  const space = createAddressSpace(layout);
  const byTarget = new Map<number, Xref[]>();
  let count = 0;

  const add = (target: number, fromSegmentId: number, ref: Omit<Xref, 'targetSegmentId'>) => {
    const resolved = space.resolve(target, fromSegmentId);
    const entry: Xref = { ...ref, targetSegmentId: resolved.status === 'ok' ? resolved.segment.id : null };
    if (!byTarget.has(target)) byTarget.set(target, []);
    byTarget.get(target)!.push(entry);
    count += 1;
  };

  const offsets = [...analysis.instructions.keys()].toSorted((a, b) => a - b);
  const decoded = new Map<number, DecodedInstruction>();
  for (const offset of offsets) {
    const info = analysis.instructions.get(offset)!;
    decoded.set(offset, decode(bytes, offset, info.address) as DecodedInstruction);
  }

  for (const offset of offsets) {
    const info = analysis.instructions.get(offset)!;
    const instruction = decoded.get(offset)!;
    const from = { segmentId: info.segmentId, address: info.address, mnemonic: instruction.mnemonic };
    if (instruction.flow === 'branch' || instruction.flow === 'jump' || instruction.flow === 'call') {
      add(instruction.target!, info.segmentId, { kind: instruction.flow, ...from });
    } else if (instruction.flow === 'jump-indirect') {
      add(instruction.reference!, info.segmentId, { kind: 'read', ...from });
    } else if (instruction.reference !== null) {
      const kind = memoryAccess(instruction.mnemonic, instruction.mode);
      if (kind !== null) add(instruction.reference!, info.segmentId, { kind, ...from, indexed: INDEXED_MODES.has(instruction.mode) });
    }
  }

  // 取地址：两条立即数加载组成一个落在段里的地址
  const used = new Set<number>();
  for (const offset of offsets) {
    const first = decoded.get(offset)!;
    if (used.has(offset) || first.mode !== 'immediate' || !IMMEDIATE_LOADS.has(first.mnemonic)) continue;
    const info = analysis.instructions.get(offset)!;
    let next = offset + info.size;
    for (let step = 0; step < ADDRESS_PAIR_WINDOW && analysis.instructions.has(next); step += 1) {
      const second = decoded.get(next)!;
      if (second.mode === 'immediate' && IMMEDIATE_LOADS.has(second.mnemonic)) {
        const candidates = [((second.operand ?? 0) << 8) | (first.operand ?? 0), ((first.operand ?? 0) << 8) | (second.operand ?? 0)];
        const target = candidates.find((value) => space.resolve(value, info.segmentId).status !== 'unmapped');
        if (target !== undefined) {
          add(target, info.segmentId, { kind: 'address', segmentId: info.segmentId, address: info.address, mnemonic: first.mnemonic, likely: true });
          used.add(offset);
          used.add(next);
        }
        break;
      }
      if (second.flow !== 'next') break;
      next += analysis.instructions.get(next)!.size;
    }
  }

  // 地址表：每个表项引用它的目标
  for (const table of analysis.tables) {
    const segment = expectSegment(layout, table.segmentId);
    table.targets.forEach((target, index) => {
      const entryOffset = table.offset + (table.kind === 'words' ? index * 2 : index);
      add(target, table.segmentId, { kind: 'table', segmentId: table.segmentId, address: segment.address + (entryOffset - segment.fileOffset) });
    });
  }
  return { byTarget, count };
}

/**
 * 查某个地址的引用。
 * @param index 索引
 * @param address 被引用的 CPU 地址
 * @param [options] 只要这个种类
 * @returns 引用，按建立顺序
 */
export function findXrefs(index: XrefIndex, address: number, { kind }: {kind?: string} = {}): Xref[] {
  const all = index.byTarget.get(address) ?? [];
  return kind === undefined ? [...all] : all.filter((ref) => ref.kind === kind);
}
