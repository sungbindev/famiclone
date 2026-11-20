// ------------------------------------------------------------------------
// 名称：entry-points.mjs
// 说明：由 CPU 地址生成入口：只有一个段覆盖该地址就用它，换页窗口里要指明段号；还有由中断向量生成入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：向量指向的地址没有映射或无法确定 bank 时不生成入口，返回在 skipped 里，由调用者提示。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { createEntry, getSegment } from '../model/index.mjs';

/** 向量名与说明 */
const VECTOR_DESCRIPTIONS = { reset: '复位向量', nmi: 'NMI 向量', irq: 'IRQ/BRK 向量' };

/**
 * 十六进制写出地址。
 * @param {number} address 地址
 * @returns {string} 如 $C000
 */
function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 由 CPU 地址创建入口。
 * @param {import('./address-space.mjs').AddressSpace} space 地址空间
 * @param {number} address CPU 地址
 * @param {{source: string, description?: string, segmentId?: number}} options 来源、说明；换页窗口里的地址要给段号
 * @returns {Readonly<import('../model/entries.mjs').Entry>} 入口
 * @throws {FcdevError} 地址没有映射、无法确定 bank（DISASM_ENTRY_AMBIGUOUS），或给出的段号不覆盖这个地址
 */
export function entryAtAddress(space, address, { source, description, segmentId }) {
  if (segmentId !== undefined) {
    if (getSegment(space.layout, segmentId) === undefined) throw new FcdevError(`入口的段 ${segmentId} 不存在`, { code: 'DISASM_ENTRY_INVALID' });
    return createEntry(space.layout, { segmentId, address, source, description });
  }
  const resolved = space.resolve(address);
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
 * @param {import('./address-space.mjs').AddressSpace} space 地址空间
 * @param {{nmi: number, reset: number, irq: number}|null} vectors 向量；没有时返回空
 * @returns {{entries: Array<Readonly<import('../model/entries.mjs').Entry>>, skipped: Array<{name: string, address: number, reason: string}>}} 入口与没能生成入口的向量
 */
export function entriesFromVectors(space, vectors) {
  const entries = [];
  const skipped = [];
  if (vectors === null) return { entries, skipped };
  for (const name of ['reset', 'nmi', 'irq']) {
    const address = vectors[name];
    try {
      entries.push(entryAtAddress(space, address, { source: 'vector', description: VECTOR_DESCRIPTIONS[name] }));
    } catch (error) {
      if (!(error instanceof FcdevError)) throw error;
      skipped.push({ name, address, reason: error.message });
    }
  }
  return { entries, skipped };
}
