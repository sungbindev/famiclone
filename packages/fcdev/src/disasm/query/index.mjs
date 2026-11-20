// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：query 目录的出口：符号索引、清单、例程、搜索、分类列表与布局替换。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：都是不读写磁盘的纯函数，操作（operations 目录）把它们和工程的读写组合起来。
// ------------------------------------------------------------------------
export { DEFAULT_LIMIT, MAX_LIMIT, findBytes, findInstructions, normalizePage, parseBytePattern, textToPattern } from './find.mjs';
export { replaceLayout } from './layout-edit.mjs';
export { LIST_CATEGORIES, listItems } from './lists.mjs';
export { DEFAULT_LISTING_LINES, renderListing } from './listing.mjs';
export { buildCallGraph, describeRoutine, enclosingRoutine, findRoutine, listRoutines, registerUsage } from './routines.mjs';
export { createSymbolIndex } from './symbol-index.mjs';
