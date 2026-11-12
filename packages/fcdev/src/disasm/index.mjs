// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：disasm 目录的对外入口：反汇编并回读校验，以及解析符号文件、CDL 等辅助函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
export { analyzeCode } from './analysis.mjs';
export { parseCdl } from './cdl.mjs';
export { disassemble } from './disassembler.mjs';
export { buildRanges, renderInfo } from './info-writer.mjs';
export { loadBinary, loadInes } from './loaders.mjs';
export { findFirstDifference, renderRoundtripConfig } from './roundtrip.mjs';
export { parseSymbols } from './symbols.mjs';
export { parseTrace } from './trace.mjs';
