// ------------------------------------------------------------------------
// 名称：index.ts
// 说明：analysis 目录的出口：地址空间、入口、静态追踪、区域生成、交叉引用、候选识别，以及对工程的追
//       踪与自动命名。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
export { createAddressSpace } from './address-space';
export { ensureAutoSymbols } from './auto-symbols';
export { buildRegions } from './build-regions';
export { scanCode } from './code-scan';
export { detectCandidates } from './detect';
export { entriesFromVectors, entryAtAddress } from './entry-points';
export { BYTE_KIND } from './kinds';
export { traceProject } from './project-trace';
export { dataRunsFromFlags, entriesFromAddresses, entriesFromCodeFlags, mappedSize } from './record-entries';
export { findAddressTables } from './tables';
export { createAnalysis, traceCode } from './tracer';
export { buildXrefs, findXrefs } from './xrefs';
