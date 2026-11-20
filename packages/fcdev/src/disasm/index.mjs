// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：disasm 目录的对外入口：指令表与解码、映像格式与运行记录的解析、布局与区域与工程模型、静态追踪与分析、源码生成与回读、符号集、十六进制视图，以及一次完成的反汇编。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：类型声明由 tsc 从 JSDoc 生成到 dist/types（npm run types:build）；其他目录只经这个文件引用 disasm。
// ------------------------------------------------------------------------
export {
  BYTE_KIND, buildRegions, buildXrefs, createAddressSpace, createAnalysis, dataRunsFromFlags, detectCandidates, ensureAutoSymbols, entriesFromAddresses, entriesFromCodeFlags, entriesFromVectors, entryAtAddress, findAddressTables, findXrefs,
  mappedSize, scanCode, traceCode, traceProject,
} from './analysis/index.mjs';
export { ADDRESSING_MODES, OPCODE_TABLE, decode, findOpcode, formatHex, formatInstruction, formatOperand, isAssemblable, memoryAccess } from './cpu/index.mjs';
export { CDL_CODE, CDL_DATA, CDL_JUMP_TARGET, CDL_SUB_ENTRY, FORMAT_NAMES, SUPPORTED_MAPPERS, detectFormat, parseCallLog, parseCdl, parseFlat, parseImage, parseInes, parseInesHeader, parsePaged, parseSbdos, parseTrace } from './formats/index.mjs';
export { generateProject } from './generate/index.mjs';
export {
  COMMENT_KINDS, EDIT_OPERATIONS, ENTRY_SOURCES, HINT_FORMS, PROJECT_VERSION, REGION_TYPES, SYMBOL_KINDS, addressToOffset, applyEdits, cloneProject, createEntry, createLayout, createProject, createRegionMap,
  findSegmentsAt, findUncovered, getSegment, isImageStale, isVerificationStale, listRegions, locateOffset, parseProject, projectDigest, regionAt, regionStats, serializeProject, setRegion, sha256,
} from './model/index.mjs';
export { indexByValue, parseSymbolSet, parseSymbols } from './profile/index.mjs';
export { recordVerification, buildStatus, verifyRoundtrip, writeGeneratedFiles } from './verify/index.mjs';
export { buildHexRows, renderHexView } from './view/index.mjs';
export { API_VERSION, OPERATIONS, OPERATION_NAMES, runOperation } from './operations/index.mjs';
export {
  LIST_CATEGORIES, buildCallGraph, createSymbolIndex, describeRoutine, enclosingRoutine, findBytes, findInstructions, findRoutine, listItems, listRoutines, parseBytePattern, registerUsage, renderListing, replaceLayout, textToPattern,
} from './query/index.mjs';
export { PROJECT_FILE, disassemble, fromProjectPath, readProject, readProjectImage, readProjectSymbolSets, toProjectPath, writeProject } from './workspace/index.mjs';
