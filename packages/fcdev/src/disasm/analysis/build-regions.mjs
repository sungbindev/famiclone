// ------------------------------------------------------------------------
// 名称：build-regions.mjs
// 说明：把静态追踪的结果转成区域表：连续的指令字节是 code 区域，字表是 words 区域，低、高字节分开的两张表是 bytes 区域。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：没有被追到的字节不标注（unknown）。区域的来源写 trace 或 table；只在传入的区域表上设置，会盖住那里原有的区域。
// ------------------------------------------------------------------------
import { createRegionMap, setRegion } from '../model/index.mjs';
import { BYTE_KIND } from './kinds.mjs';

/**
 * 由分析结果生成区域表。
 * @param {import('../model/layout.mjs').Layout} layout 布局
 * @param {import('./tracer.mjs').Analysis} analysis 分析结果
 * @param {import('../model/regions.mjs').RegionMap} [map] 要写入的区域表，默认新建
 * @returns {import('../model/regions.mjs').RegionMap} 区域表
 */
export function buildRegions(layout, analysis, map = createRegionMap(layout)) {
  const { kinds } = analysis;
  for (const segment of layout.segments) {
    const end = segment.fileOffset + segment.length;
    let runStart = -1;
    for (let offset = segment.fileOffset; offset <= end; offset += 1) {
      const isCode = offset < end && (kinds[offset] === BYTE_KIND.OPCODE || kinds[offset] === BYTE_KIND.OPERAND);
      if (isCode && runStart < 0) {
        runStart = offset;
      } else if (!isCode && runStart >= 0) {
        setRegion(map, { offset: runStart, length: offset - runStart, type: 'code', source: 'trace' });
        runStart = -1;
      }
    }
  }
  for (const table of analysis.tables) {
    if (table.kind === 'words') {
      setRegion(map, { offset: table.offset, length: table.count * 2, type: 'words', params: { adjust: table.adjust }, source: 'table' });
    } else {
      setRegion(map, { offset: table.offset, length: table.count, type: 'bytes', params: { role: 'split-address-low' }, source: 'table' });
      setRegion(map, { offset: table.highOffset, length: table.count, type: 'bytes', params: { role: 'split-address-high' }, source: 'table' });
    }
  }
  return map;
}
