// ------------------------------------------------------------------------
// 名称：address-space.ts
// 说明：地址空间：把一个 CPU 地址解析成布局里的段，处理换页窗口里同一地址有多个 bank 的情况。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：规则：只有一个段覆盖该地址就是它；有多个 bank 覆盖时，来源所在的段如果是其中之一就留在这个
//       bank（同一个窗口里的跳转不换页），否则无法确定目标 bank，返回 ambiguous，由调用者登记成待确
//       认的跨 bank 转移。
// ------------------------------------------------------------------------
import { findSegmentsAt } from '../model/index';
import type { Segment, Layout } from '../model/layout';

export type Resolution = {status: 'ok', segment: Segment} | {status: 'unmapped'} | {status: 'ambiguous', candidates: Array<Segment>};

export interface AddressSpace {
  /** 布局 */
  layout: Layout;
  /** 解析 CPU 地址 */
  resolve: (address: number, fromSegmentId?: number) => Resolution;
}

/**
 * 创建地址空间。
 * @param layout 布局
 * @returns 地址空间
 */
export function createAddressSpace(layout: Layout): AddressSpace {
  return {
    layout,
    resolve(address, fromSegmentId) {
      const candidates = findSegmentsAt(layout, address);
      if (candidates.length === 0) return { status: 'unmapped' };
      if (candidates.length === 1) return { status: 'ok', segment: candidates[0]! };
      const same = candidates.find((segment) => segment.id === fromSegmentId);
      return same === undefined ? { status: 'ambiguous', candidates } : { status: 'ok', segment: same };
    },
  };
}
