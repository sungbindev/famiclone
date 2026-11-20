// ------------------------------------------------------------------------
// 名称：pipeline.mjs
// 说明：一次完成的反汇编：读映像、登记入口与符号集、静态追踪、自动命名、生成源码工程、写盘、回读校验，并保存工程文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：fcdev disasm 命令的实现；Copilot 工具按步骤调用 operations 目录的操作，不经过这里。
//       回读不一致时工程目录与 disasm.json 仍然保留（记着失败的回读结果），再抛出错误说明第一处差异。
// ------------------------------------------------------------------------
import { mkdirSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { ensureAutoSymbols, mappedSize, traceProject } from '../analysis/index.mjs';
import { generateProject } from '../generate/index.mjs';
import { regionStats } from '../model/index.mjs';
import { indexByValue, parseSymbolSet } from '../profile/index.mjs';
import { buildStatus, recordVerification, verifyRoundtrip } from '../verify/index.mjs';
import { openImageProject } from './open-project.mjs';
import { writeProject } from './project-store.mjs';

/**
 * @typedef {object} DisassemblyReport
 * @property {string} projectDir 工程目录
 * @property {string} projectFile 工程文件
 * @property {string} description 输入的来源说明
 * @property {number} size 映像文件的字节数
 * @property {number} mappedBytes 映像里落在 CPU 地址空间（布局的段）里的字节数
 * @property {number} instructions 本次追踪标出的指令条数
 * @property {number} codeBytes 区域类型为 code 的字节数
 * @property {string[]} warnings 追踪与生成中的警告
 * @property {boolean} verified 回读与原映像逐字节一致
 * @property {import('../verify/status.mjs').Status} status 进度报告
 */

/**
 * 十六进制写出地址。
 * @param {number} address 地址
 * @returns {string} 如 $C000
 */
function hex(address) {
  return `$${address.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * 反汇编一个映像并回读校验。
 * @param {{inputFile: string, outDir: string, format?: string, base?: number, entries?: number[], cdlFile?: string, traceFile?: string, symbolsFile?: string, symbolSetFiles?: string[], toolchain: import('../../toolchain/index.mjs').Toolchain}} input 输入映像、工程目录、格式、平铺格式的加载地址、手工入口、CDL、指令轨迹、符号文件、符号集文件与工具链
 * @returns {Promise<DisassemblyReport>} 报告
 * @throws {FcdevError} 输入不合法、没有入口、汇编链接失败，或回读不一致
 */
export async function disassemble({ inputFile, outDir, format, base, entries = [], cdlFile, traceFile, symbolsFile, symbolSetFiles = [], toolchain }) {
  const projectDir = path.resolve(outDir);
  mkdirSync(projectDir, { recursive: true });
  const opened = openImageProject({ inputFile, projectDir, format, base, entries, cdlFile, traceFile, symbolsFile, symbolSetFiles, requireEntries: true });
  const { bytes, parsed, sets, warnings } = opened;

  const traceResult = traceProject(opened.project, bytes);
  warnings.push(...traceResult.analysis.warnings.map((warning) => warning.message));
  const constants = sets.flatMap((set) => parseSymbolSet(set.text));
  let project = ensureAutoSymbols(traceResult.project, bytes, { externalNames: indexByValue(constants) }).project;

  const generated = generateProject(project, bytes, { symbolSets: new Map(sets.map((set) => [set.name, set.text])) });
  warnings.push(...generated.warnings.map((warning) => warning.message));
  const projectFile = writeProject(projectDir, project);
  const result = await verifyRoundtrip({ project, bytes, generated, projectDir, toolchain });
  project = recordVerification(project, result);
  writeProject(projectDir, project);

  if (!result.ok) {
    const { address, segmentId, symbol } = result.firstDifference;
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
