// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：model 目录的出口：映像布局、区域、入口，以及工程（符号、注释、操作数提示、模块）与批量编辑。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
export { EDIT_OPERATIONS, applyEdits } from './edits.mjs';
export { ENTRY_SOURCES, createEntry } from './entries.mjs';
export { addressToOffset, createLayout, findSegmentsAt, findUncovered, getSegment, locateOffset } from './layout.mjs';
export { PROJECT_VERSION, cloneProject, createProject, isImageStale, isVerificationStale, parseProject, projectDigest, serializeProject, sha256 } from './project.mjs';
export { REGION_TYPES, createRegionMap, listRegions, regionAt, regionStats, setRegion } from './regions.mjs';
export { COMMENT_KINDS, HINT_FORMS, SYMBOL_KINDS, assertModulesDisjoint, assertSymbolName, normalizeComment, normalizeHint, normalizeModule, normalizeSymbol, resolveSegmentId } from './symbols.mjs';
