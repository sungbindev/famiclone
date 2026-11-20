// ------------------------------------------------------------------------
// 名称：symbol-index.mjs
// 说明：符号的查找索引：按名字、按（段，地址）查工程里的符号，以及把一个 CPU 地址落实成符号名（布局内的符号、布局外的符号、符号集里的常量）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：给查看与查询用，不修改工程；符号集的常量只在地址落在布局外时使用，与生成源码时的规则一致。
// ------------------------------------------------------------------------
import { createAddressSpace } from '../analysis/index.mjs';
import { indexByValue } from '../profile/index.mjs';

/**
 * @typedef {object} SymbolIndex
 * @property {Map<string, import('../model/symbols.mjs').Symbol>} byName 名字到符号
 * @property {(segmentId: number, address: number) => object|null} at 布局内某个位置上的符号
 * @property {(address: number, fromSegmentId?: number) => string|null} nameFor 一个 CPU 地址对应的符号名；换页窗口里按来源所在的 bank 解析，找不到时为 null
 * @property {ReturnType<typeof createAddressSpace>} space 地址空间
 */

/**
 * 建立符号索引。
 * @param {import('../model/project.mjs').Project} project 工程
 * @param {Array<{name: string, value: number}>} [constants] 符号集里的常量
 * @returns {SymbolIndex} 索引
 */
export function createSymbolIndex(project, constants = []) {
  const space = createAddressSpace(project.layout);
  const byName = new Map(project.symbols.map((symbol) => [symbol.name, symbol]));
  const inLayout = new Map();
  const outside = new Map();
  for (const symbol of project.symbols) {
    if (symbol.segmentId === null) outside.set(symbol.address, symbol);
    else inLayout.set(`${symbol.segmentId}:${symbol.address}`, symbol);
  }
  const setNames = indexByValue(constants);

  const at = (segmentId, address) => inLayout.get(`${segmentId}:${address}`) ?? null;
  const nameFor = (address, fromSegmentId) => {
    const resolved = space.resolve(address, fromSegmentId);
    if (resolved.status === 'ok') return at(resolved.segment.id, address)?.name ?? null;
    if (resolved.status === 'ambiguous') return null;
    return outside.get(address)?.name ?? setNames.get(address) ?? null;
  };
  return { byName, at, nameFor, space };
}
