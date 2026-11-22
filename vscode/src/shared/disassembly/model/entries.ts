// ------------------------------------------------------------------------
// 名称：entries.ts
// 说明：入口模型：静态追踪的起点，由段号加 CPU 地址确定，并记录来源与说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：换页窗口里同一个 CPU 地址在不同 bank 是不同的位置，所以入口必须带段号；只知道地址时用
//       analysis 目录的 entryAtAddress 解析。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { getSegment } from './layout';
import type { Layout } from './layout';

/** 入口的来源：向量、手工、指令跟踪、CDL、调用记录 */
export const ENTRY_SOURCES = Object.freeze(['vector', 'manual', 'trace', 'cdl', 'call-record']);

export interface Entry {
  /** 段号 */
  segmentId: number;
  /** CPU 地址 */
  address: number;
  /** 来源，见 ENTRY_SOURCES */
  source: string;
  /** 说明 */
  description: string;
}

/**
 * 创建入口并检查它落在段内。
 * @param layout 布局
 * @param input 入口
 * @returns 入口
 * @throws FcdevError 段不存在、地址不在段内、来源不合法
 */
export function createEntry(layout: Layout, { segmentId, address, source, description = '' }: {segmentId: number, address: number, source: string, description?: string}): Readonly<Entry> {
  const segment = getSegment(layout, segmentId);
  if (segment === undefined) throw new FcdevError(`入口的段 ${segmentId} 不存在`, { code: 'DISASM_ENTRY_INVALID' });
  if (!Number.isInteger(address) || address < segment.address || address >= segment.address + segment.length) {
    throw new FcdevError(`入口地址 $${Number(address).toString(16).toUpperCase()} 不在段 ${segmentId} 里`, { code: 'DISASM_ENTRY_INVALID' });
  }
  if (!ENTRY_SOURCES.includes(source)) throw new FcdevError(`未知的入口来源：${source}`, { code: 'DISASM_ENTRY_INVALID', hint: `可用的来源：${ENTRY_SOURCES.join('、')}` });
  if (typeof description !== 'string') throw new FcdevError('入口的说明必须是字符串', { code: 'DISASM_ENTRY_INVALID' });
  return Object.freeze({ segmentId, address, source, description });
}
