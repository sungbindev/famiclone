// ------------------------------------------------------------------------
// 名称：pipeline.ts
// 说明：一次完成的反汇编：读映像、登记入口与符号集、静态追踪、自动命名、生成源码工程、写盘、回读校
//       验，并保存工程文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：“反汇编映像文件”命令的实现；Copilot 工具按步骤调用 operations 目录的操作，不经过这里。
//       回读不一致时工程目录与 disasm.json 仍然保留（记着失败的回读结果），再抛出错误
//       说明第一处差异。
// ------------------------------------------------------------------------
import * as path from 'node:path';
import type { DisasmEnvironment } from '../../../contracts/disassembly/disasm-environment';
import type { DisassembleInput, DisassemblyReport } from '../../../contracts/disassembly/disasm-operations';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { ensureAutoSymbols, mappedSize, traceProject } from '../../../shared/disassembly/analysis/index';
import { generateProject } from '../../../shared/disassembly/generate/index';
import { regionStats } from '../../../shared/disassembly/model/index';
import { indexByValue, parseSymbolSet } from '../../../shared/disassembly/profile/index';
import { buildStatus } from '../../../shared/disassembly/verify/status';
import { recordVerification, verifyRoundtrip } from '../verify/roundtrip';
import { openImageProject } from './open-project';
import { writeProject } from './project-store';

/**
 * 十六进制写出地址。
 * @param address 地址
 * @returns 如 $C000
 */
function hex(address: number): string {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 反汇编一个映像并回读校验。
 * @param environment 文件系统、工具链定位与工具运行
 * @param input 输入映像、工程目录、格式、平铺格式的加载地址、手工入口、CDL、指令轨迹、符号文件与符
 *   号集文件
 * @returns 报告
 * @throws FcdevError 输入不合法、没有入口、汇编链接失败，或回读不一致
 */
export async function disassemble({ files, toolchain, tools }: DisasmEnvironment, { inputFile, outDir, format, base, entries = [], cdlFile, traceFile, symbolsFile, symbolSetFiles = [] }: DisassembleInput): Promise<DisassemblyReport> {
  const projectDir = path.resolve(outDir);
  files.ensureDirectory(projectDir);
  const opened = openImageProject(files, { inputFile, projectDir, format, base, entries, cdlFile, traceFile, symbolsFile, symbolSetFiles, requireEntries: true });
  const { bytes, parsed, sets, warnings } = opened;

  const traceResult = traceProject(opened.project, bytes);
  warnings.push(...traceResult.analysis.warnings.map((warning) => warning.message));
  const constants = sets.flatMap((set) => parseSymbolSet(set.text));
  let project = ensureAutoSymbols(traceResult.project, bytes, { externalNames: indexByValue(constants) }).project;

  const generated = generateProject(project, bytes, { symbolSets: new Map(sets.map((set) => [set.name, set.text])) });
  warnings.push(...generated.warnings.map((warning) => warning.message));
  const projectFile = writeProject(files, projectDir, project);
  const result = await verifyRoundtrip({ files, tools, project, bytes, generated, projectDir, toolchain: toolchain.locate() });
  project = recordVerification(project, result);
  writeProject(files, projectDir, project);

  if (!result.ok) {
    const { address, segmentId, symbol } = result.firstDifference!;
    throw new FcdevError(`回读校验不一致：段 ${segmentId} 的 ${hex(address)} 起与原映像不同${symbol === null ? '' : `（在符号 ${symbol} 之后）`}`, {
      code: 'DISASM_ROUNDTRIP_MISMATCH',
      hint: `工程保留在 ${projectDir}，可对照 verify 目录检查`,
    });
  }
  return {
    projectDir,
    projectFile,
    description: parsed.description,
    size: bytes.length,
    mappedBytes: mappedSize(parsed.layout),
    instructions: traceResult.summary.instructions,
    codeBytes: regionStats(project.regions).code ?? 0,
    warnings,
    verified: true,
    status: buildStatus(project, bytes),
  };
}
