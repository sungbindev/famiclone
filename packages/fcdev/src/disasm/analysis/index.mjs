// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：analysis 目录的出口：地址空间、入口、静态追踪、区域生成、交叉引用、候选识别。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
export { createAddressSpace } from './address-space.mjs';
export { buildRegions } from './build-regions.mjs';
export { detectCandidates } from './detect.mjs';
export { entriesFromVectors, entryAtAddress } from './entry-points.mjs';
export { BYTE_KIND } from './kinds.mjs';
export { findAddressTables } from './tables.mjs';
export { createAnalysis, traceCode } from './tracer.mjs';
export { buildXrefs, findXrefs } from './xrefs.mjs';
