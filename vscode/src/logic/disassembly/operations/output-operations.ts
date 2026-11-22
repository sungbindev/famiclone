// ------------------------------------------------------------------------
// 名称：output-operations.ts
// 说明：生成与校验：generate（生成源码工程并写盘）、verify（重新汇编链接并回读比较，结果记进工
//       程）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：回读不一致时给出第一处差异所在的段、地址、符号、区域类型与例程；工具链用随扩展发布的 cc65。
// ------------------------------------------------------------------------
import type { DisasmEnvironment } from '../../../contracts/disassembly/disasm-environment';
import { scanCode } from '../../../shared/disassembly/analysis/index';
import { generateProject } from '../../../shared/disassembly/generate/index';
import { addressToOffset, getSegment, regionAt } from '../../../shared/disassembly/model/index';
import { enclosingRoutine, listRoutines } from '../../../shared/disassembly/query/index';
import { buildStatus } from '../../../shared/disassembly/verify/status';
import { recordVerification, verifyRoundtrip, writeGeneratedFiles } from '../verify/roundtrip';
import { loadContext, saveContext } from './context';
import { projectDirectory } from '../../../shared/disassembly/operations/params';

/**
 * 生成源码工程并写进工程目录。
 * @param environment 运行环境
 * @param params projectDir
 * @returns 写出的文件、模块、各段的输出文件与警告
 */
export function generate({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const generated = generateProject(context.project, context.bytes, { symbolSets: context.symbolSetTexts });
  writeGeneratedFiles(files, context.dir, generated.files);
  return {
    files: [...generated.files.keys()],
    modules: generated.modules,
    segments: generated.segments,
    warnings: generated.warnings.map((warning) => warning.message),
  };
}

/**
 * 回读校验：重新生成、汇编、链接，逐段与映像比较，并把结果记进工程。
 * @param environment 运行环境
 * @param params projectDir
 * @returns 是否一致；不一致时第一处差异的位置与上下文；进度里的回读状态
 */
export async function verify(environment: DisasmEnvironment, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { files, tools, toolchain: toolchainLocator } = environment;
  const context = loadContext(files, projectDirectory(params));
  const { project, bytes } = context;
  const generated = generateProject(project, bytes, { symbolSets: context.symbolSetTexts });
  const toolchain = toolchainLocator.locate();
  const result = await verifyRoundtrip({ files, tools, project, bytes, generated, projectDir: context.dir, toolchain });
  const recorded = recordVerification(project, result);
  saveContext(files, context, recorded);

  let difference = null;
  if (!result.ok) {
    const found = result.firstDifference!;
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
