// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：model 目录的出口：映像布局、区域、入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：符号、注释、操作数提示、模块等工程内容在阶段 3 加入。
// ------------------------------------------------------------------------
export { ENTRY_SOURCES, createEntry } from './entries.mjs';
export { addressToOffset, createLayout, findSegmentsAt, findUncovered, getSegment, locateOffset } from './layout.mjs';
export { REGION_TYPES, createRegionMap, listRegions, regionAt, regionStats, setRegion } from './regions.mjs';
