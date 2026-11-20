// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：shared 目录的对外入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：其他目录只能经这个文件引用 shared。
// ------------------------------------------------------------------------
export { FcdevError } from './errors.mjs';
export { createParamChecks } from './param-checks.mjs';
export { BOOT_STEP_NAMES, PROFILE_FILE, listBuiltinProfiles, normalizeProfile, readTargetProfile, selectProfileInput } from './target-profile.mjs';
export { DEFAULT_TOOL_TIMEOUT_MS, runProcess } from './process.mjs';
export { ensureDirectory, getModifiedTime, isDirectory, readJsonFile } from './fs-utils.mjs';
