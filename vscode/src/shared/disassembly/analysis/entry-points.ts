// ------------------------------------------------------------------------
// 名称：entry-points.ts
// 说明：由 CPU 地址生成入口：只有一个段覆盖该地址就用它，换页窗口里要指明段号；还有由中断向量生成入
//       口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：向量指向的地址没有映射或无法确定 bank 时不生成入口，返回在 skipped 里，由调用者提示。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { createEntry, getSegment } from '../model/index';
import type { Entry } from '../model/entries';
import type { AddressSpace } from './address-space';

/** 向量名与说明 */
const VECTOR_DESCRIPTIONS = { reset: '复位向量', nmi: 'NMI 向量', irq: 'IRQ/BRK 向量' };

/**
 * 十六进制写出地址。
 * @param address 地址
 * @returns 如 $C000
 */
function hex(address: number): string {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 由 CPU 地址创建入口。
 * @param space 地址空间
 * @param address CPU 地址
 * @param options 来源、说明；换页窗口里的地址要给段号，或给 fromSegmentId（地址在其中某个段里读到，
 *   窗口里不换 bank）
 * @returns 入口
 * @throws FcdevError 地址没有映射、无法确定 bank（DISASM_ENTRY_AMBIGUOUS），或给出的段号不覆盖这个
 *   地址
 */
export function entryAtAddress(space: AddressSpace, address: number, { source, description, segmentId, fromSegmentId }: {source: string, description?: string, segmentId?: number, fromSegmentId?: number}): Readonly<Entry> {
  if (segmentId !== undefined) {
    if (getSegment(space.layout, segmentId) === undefined) throw new FcdevError(`入口的段 ${segmentId} 不存在`, { code: 'DISASM_ENTRY_INVALID' });
    return createEntry(space.layout, { segmentId, address, source, description });
  }
  const resolved = space.resolve(address, fromSegmentId);
  if (resolved.status === 'unmapped') throw new FcdevError(`入口地址 ${hex(address)} 没有映射到任何段`, { code: 'DISASM_ENTRY_INVALID' });
  if (resolved.status === 'ambiguous') {
    throw new FcdevError(`入口地址 ${hex(address)} 在多个 bank 里都有映射（段 ${resolved.candidates.map((segment) => segment.id).join('、')}）`, {
      code: 'DISASM_ENTRY_AMBIGUOUS',
      hint: '给出入口所在的段号',
    });
  }
  return createEntry(space.layout, { segmentId: resolved.segment.id, address, source, description });
}

/**
 * 由中断向量创建入口。
 * @param space 地址空间
 * @param vectors 向量；没有时返回空
 * @param [fromSegmentId] 向量是在哪个段里读到的；指向换页窗口的向量用它确定 bank
 * @returns 入口与没能生成入口的向量
 */
export function entriesFromVectors(space: AddressSpace, vectors: {nmi: number, reset: number, irq: number}|null, fromSegmentId?: number): {entries: Array<Readonly<Entry>>, skipped: Array<{name: string, address: number, reason: string}>} {
  const entries: Array<Readonly<Entry>> = [];
  const skipped: Array<{name: string, address: number, reason: string}> = [];
  if (vectors === null) return { entries, skipped };
  for (const name of ['reset', 'nmi', 'irq'] as const) {
    const address = vectors[name];
    try {
      entries.push(entryAtAddress(space, address, { source: 'vector', description: VECTOR_DESCRIPTIONS[name], fromSegmentId }));
    } catch (error) {
      if (!(error instanceof FcdevError)) throw error;
      skipped.push({ name, address, reason: error.message });
    }
  }
  return { entries, skipped };
}
