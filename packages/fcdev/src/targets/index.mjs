// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：targets 目录的对外入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
export { applyNameRule } from './name-rules.mjs';
export {
  BUILTIN_TARGETS_DIR,
  listTargetNames,
  loadTarget,
  parseTarget,
  resolveTargetOptions,
  resolveTargetPath,
} from './target-registry.mjs';
