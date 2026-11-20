// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：disasm 目录的对外入口：反汇编并回读校验，以及解析符号文件、CDL 等辅助函数；新核心的指令表与解码、布局、映像格式解析器、十六进制视图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：平铺在本目录里的文件是基于 da65 的旧实现，阶段 4 用新核心重写后删除；cpu、model、formats、view 是新核心。
// ------------------------------------------------------------------------
export { analyzeCode } from './analysis.mjs';
export { parseCdl } from './cdl.mjs';
export { ADDRESSING_MODES, OPCODE_TABLE, decode, findOpcode, formatHex, formatInstruction, formatOperand, isAssemblable } from './cpu/index.mjs';
export { disassemble } from './disassembler.mjs';
export { FORMAT_NAMES, parseFlat, parseImage, parseInes, parseInesHeader } from './formats/index.mjs';
export { buildRanges, renderInfo } from './info-writer.mjs';
export { loadBinary, loadInes } from './loaders.mjs';
export { addressToOffset, createLayout, findSegmentsAt, findUncovered, getSegment, locateOffset } from './model/index.mjs';
export { findFirstDifference, renderRoundtripConfig } from './roundtrip.mjs';
export { parseSymbols } from './symbols.mjs';
export { parseTrace } from './trace.mjs';
export { buildHexRows, renderHexView } from './view/index.mjs';
