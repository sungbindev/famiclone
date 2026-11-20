// ------------------------------------------------------------------------
// 名称：output-operations.mjs
// 说明：生成与校验：generate（生成源码工程并写盘）、verify（重新汇编链接并回读比较，结果记进工程）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：回读不一致时给出第一处差异所在的段、地址、符号、区域类型与例程；工具链先按参数 toolchain 指定的目录，再按环境变量与随包的工具链。
// ------------------------------------------------------------------------
import { scanCode } from '../analysis/index.mjs';
import { generateProject } from '../generate/index.mjs';
import { addressToOffset, getSegment, regionAt } from '../model/index.mjs';
import { enclosingRoutine, listRoutines } from '../query/index.mjs';
import { buildStatus, recordVerification, verifyRoundtrip, writeGeneratedFiles } from '../verify/index.mjs';
import { locateToolchain } from '../../toolchain/index.mjs';
import { loadContext, saveContext } from './context.mjs';
import { optionalString, projectDirectory } from './params.mjs';

/**
 * 生成源码工程并写进工程目录。
 * @param {Record<string, unknown>} params projectDir
 * @returns {object} 写出的文件、模块、各段的输出文件与警告
 */
export function generate(params) {
  const context = loadContext(projectDirectory(params));
  const generated = generateProject(context.project, context.bytes, { symbolSets: context.symbolSetTexts });
  writeGeneratedFiles(context.dir, generated.files);
  return {
    files: [...generated.files.keys()],
    modules: generated.modules,
    segments: generated.segments,
    warnings: generated.warnings.map((warning) => warning.message),
  };
}

/**
 * 回读校验：重新生成、汇编、链接，逐段与映像比较，并把结果记进工程。
 * @param {Record<string, unknown>} params projectDir；可选 toolchain（cc65 的根目录）
 * @param {{env?: NodeJS.ProcessEnv}} [options] env 为环境变量，默认当前进程的
 * @returns {Promise<object>} 是否一致；不一致时第一处差异的位置与上下文；进度里的回读状态
 */
export async function verify(params, { env = process.env } = {}) {
  const context = loadContext(projectDirectory(params));
  const { project, bytes } = context;
  const generated = generateProject(project, bytes, { symbolSets: context.symbolSetTexts });
  const toolchain = locateToolchain({ configured: optionalString(params, 'toolchain'), env });
  const result = await verifyRoundtrip({ project, bytes, generated, projectDir: context.dir, toolchain });
  const recorded = recordVerification(project, result);
  saveContext(context, recorded);

  let difference = null;
  if (!result.ok) {
    const found = result.firstDifference;
    const segment = getSegment(project.layout, found.segmentId);
    const offset = segment === undefined ? null : addressToOffset(segment, found.address);
    const routines = listRoutines(project, scanCode(project, bytes));
    difference = {
      ...found,
      region: offset === null ? null : (regionAt(project.regions, offset)?.type ?? 'unknown'),
      routine: enclosingRoutine(routines, found.segmentId, found.address)?.name ?? null,
    };
  }
  return { ok: result.ok, firstDifference: difference, warnings: generated.warnings.map((warning) => warning.message), verification: buildStatus(recorded, bytes).verification };
}
