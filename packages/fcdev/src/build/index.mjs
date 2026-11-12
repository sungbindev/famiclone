// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：build 目录的对外入口：解析构建路径并构建程序。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import { locateToolchain } from '../toolchain/index.mjs';
import { createBuildPlan } from './build-plan.mjs';
import { buildProgram } from './program-builder.mjs';

export { createBuildPlan } from './build-plan.mjs';
export { expandBankedLinkConfig, getBankCount, resolveLinkSetup } from './bank-link-config.mjs';
export { parseDependencyFile } from './dependency-file.mjs';
export { buildDiskImage } from './disk-builder.mjs';
export { buildProgram } from './program-builder.mjs';
export { collectSources, getSourceLanguage } from './source-collector.mjs';
export { buildAssemblerArguments, buildCompilerArguments, buildLinkerArguments } from './tool-arguments.mjs';

/**
 * 解析路径并构建。
 * @param {{pathArg?: string, cwd: string, targetOverride?: string, targetsDir?: string, env?: NodeJS.ProcessEnv}} input 构建路径、终端目录与目标覆盖
 * @param {import('./program-builder.mjs').Reporter} [reporter] 汇报器
 * @returns {Promise<{plan: import('./build-plan.mjs').BuildPlan, result: import('./program-builder.mjs').BuildResult}>} 计划与结果
 */
export async function buildPath(input, reporter) {
  const plan = createBuildPlan(input);
  const toolchain = locateToolchain({ configured: plan.config.toolchain, env: input.env });
  const result = await buildProgram(plan, toolchain, reporter);
  return { plan, result };
}
