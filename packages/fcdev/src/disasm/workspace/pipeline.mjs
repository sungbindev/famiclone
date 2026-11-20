// ------------------------------------------------------------------------
// 名称：pipeline.mjs
// 说明：一次完成的反汇编：读映像、登记入口与符号集、静态追踪、自动命名、生成源码工程、写盘、回读校验，并保存工程文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：fcdev disasm 命令的实现；Copilot 工具按步骤调用各目录的函数，不经过这里。
//       回读不一致时工程目录与 disasm.json 仍然保留（记着失败的回读结果），再抛出错误说明第一处差异。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { createAddressSpace, ensureAutoSymbols, entriesFromAddresses, entriesFromCodeFlags, entriesFromVectors, entryAtAddress, mappedSize, traceProject } from '../analysis/index.mjs';
import { detectFormat, parseCdl, parseImage, parseTrace } from '../formats/index.mjs';
import { generateProject } from '../generate/index.mjs';
import { applyEdits, createProject, regionStats } from '../model/index.mjs';
import { indexByValue, parseSymbolSet, parseSymbols } from '../profile/index.mjs';
import { buildStatus, recordVerification, verifyRoundtrip } from '../verify/index.mjs';
import { toProjectPath, writeProject } from './project-store.mjs';

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
 * 去掉重复的入口，保留先出现的。
 * @param {Array<Readonly<import('../model/entries.mjs').Entry>>} entries 入口
 * @returns {Array<Readonly<import('../model/entries.mjs').Entry>>} 去重后的入口
 */
function uniqueEntries(entries) {
  const seen = new Set();
  return entries.filter((entry) => {
    const key = `${entry.segmentId}:${entry.address}`;
    if (seen.has(key)) return false;
    seen.add(key);
    return true;
  });
}

/**
 * 登记符号集：文件名取自路径，不能重名。
 * @param {string} projectDir 工程目录
 * @param {string[]} files 符号集文件的绝对路径
 * @returns {Array<{name: string, path: string, text: string}>} 符号集
 * @throws {FcdevError} 读不到文件，或两个文件重名
 */
function loadSymbolSets(projectDir, files) {
  const sets = [];
  for (const file of files) {
    const name = path.basename(file);
    if (sets.some((set) => set.name === name)) throw new FcdevError(`有两个符号集都叫 ${name}`, { code: 'DISASM_SYMBOL_SET_DUPLICATE' });
    let text;
    try {
      text = readFileSync(file, 'utf8');
    } catch (error) {
      throw new FcdevError(`读不到符号集 ${file}`, { code: 'DISASM_SYMBOL_SET_MISSING', cause: error });
    }
    sets.push({ name, path: toProjectPath(projectDir, file), text });
  }
  return sets;
}

/**
 * 汇总登记入口时没能生成入口的记录地址。
 * @param {string} label 记录的名称
 * @param {Array<{address: number}>} skipped 跳过的地址
 * @returns {string[]} 警告，没有跳过时为空
 */
function skippedWarnings(label, skipped) {
  if (skipped.length === 0) return [];
  const first = skipped.slice(0, 3).map((item) => hex(item.address)).join('、');
  return [`${label}里有 ${skipped.length} 个地址不在映像里或无法确定 bank（如 ${first}），已忽略`];
}

/**
 * 反汇编一个映像并回读校验。
 * @param {{inputFile: string, outDir: string, format?: string, base?: number, entries?: number[], cdlFile?: string, traceFile?: string, symbolsFile?: string, symbolSetFiles?: string[], toolchain: import('../../toolchain/index.mjs').Toolchain}} input 输入映像、工程目录、格式、平铺格式的加载地址、手工入口、CDL、指令轨迹、符号文件、符号集文件与工具链
 * @returns {Promise<DisassemblyReport>} 报告
 * @throws {FcdevError} 输入不合法、没有入口、汇编链接失败，或回读不一致
 */
export async function disassemble({ inputFile, outDir, format, base, entries = [], cdlFile, traceFile, symbolsFile, symbolSetFiles = [], toolchain }) {
  const buffer = readFileSync(inputFile);
  const kind = format ?? detectFormat(buffer);
  const formatOptions = kind === 'flat' ? { address: base } : {};
  if (kind === 'flat' && base === undefined) {
    throw new FcdevError('平铺二进制需要指定加载地址', { code: 'ARGUMENT_MISSING', hint: '用 --base <十六进制地址> 指定，如 --base 8000' });
  }
  const parsed = parseImage(kind, buffer, formatOptions);
  const space = createAddressSpace(parsed.layout);
  const warnings = [];

  const vectors = entriesFromVectors(space, parsed.vectors);
  warnings.push(...vectors.skipped.map((item) => `${item.name} 向量指向 ${hex(item.address)}：${item.reason}`));
  const manual = entries.map((address) => entryAtAddress(space, address, { source: 'manual', description: '命令行指定' }));
  const traced = traceFile === undefined ? { entries: [], skipped: [] } : entriesFromAddresses(space, parseTrace(readFileSync(traceFile, 'utf8')), { source: 'trace', description: '指令轨迹' });
  warnings.push(...skippedWarnings('指令轨迹', traced.skipped));
  const recorded = cdlFile === undefined ? [] : entriesFromCodeFlags(parsed.layout, parseCdl(readFileSync(cdlFile), mappedSize(parsed.layout)), { source: 'cdl', description: '代码数据日志' });
  const allEntries = uniqueEntries([...vectors.entries, ...manual, ...traced.entries, ...recorded]);
  if (allEntries.length === 0) {
    throw new FcdevError('没有入口可供追踪代码', { code: 'ARGUMENT_MISSING', hint: '用 --entry <十六进制地址> 指定入口，或用 --cdl、--trace 提供运行记录' });
  }

  const projectDir = path.resolve(outDir);
  mkdirSync(projectDir, { recursive: true });
  const sets = loadSymbolSets(projectDir, symbolSetFiles);
  let project = createProject({
    imagePath: toProjectPath(projectDir, path.resolve(inputFile)),
    bytes: buffer,
    format: kind,
    formatOptions,
    layout: parsed.layout,
    entries: allEntries,
    symbolSets: sets.map(({ name, path: stored }) => ({ name, path: stored })),
  });
  if (symbolsFile !== undefined) {
    // 同一地址有多个名字时只用第一个
    const defines = [];
    for (const symbol of parseSymbols(readFileSync(symbolsFile, 'utf8'))) {
      if (!defines.some((item) => item.address === symbol.address)) defines.push(symbol);
    }
    project = applyEdits(project, defines.map(({ name, address }) => ({ op: 'defineSymbol', name, address }))).project;
  }

  const traceResult = traceProject(project, buffer);
  warnings.push(...traceResult.analysis.warnings.map((warning) => warning.message));
  const constants = sets.flatMap((set) => parseSymbolSet(set.text));
  project = ensureAutoSymbols(traceResult.project, buffer, { externalNames: indexByValue(constants) }).project;

  const generated = generateProject(project, buffer, { symbolSets: new Map(sets.map((set) => [set.name, set.text])) });
  warnings.push(...generated.warnings.map((warning) => warning.message));
  const projectFile = writeProject(projectDir, project);
  const result = await verifyRoundtrip({ project, bytes: buffer, generated, projectDir, toolchain });
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
    size: buffer.length,
    mappedBytes: mappedSize(parsed.layout),
    instructions: traceResult.summary.instructions,
    codeBytes: regionStats(project.regions).code ?? 0,
    warnings,
    verified: true,
    status: buildStatus(project, buffer),
  };
}
