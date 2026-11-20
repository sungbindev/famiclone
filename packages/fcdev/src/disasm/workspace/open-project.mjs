// ------------------------------------------------------------------------
// 名称：open-project.mjs
// 说明：由映像文件创建反汇编工程：解析格式与布局，登记入口与符号集，读入符号文件；不追踪、不生成、不写盘。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：一次完成的反汇编（pipeline.mjs）与 open 操作共用；入口来自中断向量、手工指定、指令轨迹与 CDL，去重后保留先出现的。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { createAddressSpace, entriesFromAddresses, entriesFromCodeFlags, entriesFromVectors, entryAtAddress, mappedSize } from '../analysis/index.mjs';
import { detectFormat, parseCdl, parseImage, parseTrace } from '../formats/index.mjs';
import { applyEdits, createProject } from '../model/index.mjs';
import { parseSymbols } from '../profile/index.mjs';
import { toProjectPath } from './project-store.mjs';

/**
 * @typedef {object} OpenedImage
 * @property {import('../model/project.mjs').Project} project 新工程
 * @property {Buffer} bytes 映像字节
 * @property {import('../formats/index.mjs').ParsedImage} parsed 格式解析结果
 * @property {Array<{name: string, path: string, text: string}>} sets 符号集（含内容）
 * @property {string[]} warnings 警告
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
export function loadSymbolSets(projectDir, files) {
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
 * 由映像文件创建工程。
 * @param {{inputFile: string, projectDir: string, format?: string, formatOptions?: Record<string, unknown>, base?: number, segments?: Array<object>, entries?: Array<number|{address: number, segmentId?: number}>, cdlFile?: string, traceFile?: string, symbolsFile?: string, symbolSetFiles?: string[], profile?: {name: string, input: string, graphicsPorts: number[]}, useVectors?: boolean, requireEntries?: boolean}} input
 *   映像文件、工程目录、格式、格式选项（分页格式的页布局等）、平铺格式的加载地址或段列表、手工入口、CDL、指令轨迹、符号文件、符号集文件、目标描述的记录、是否取中断向量与格式声明的入口、没有入口时是否报错
 * @returns {OpenedImage} 新工程与相关数据
 * @throws {FcdevError} 输入不合法，或要求有入口而没有
 */
export function openImageProject({ inputFile, projectDir, format, formatOptions: givenOptions, base, segments, entries = [], cdlFile, traceFile, symbolsFile, symbolSetFiles = [], profile, useVectors = true, requireEntries = false }) {
  const bytes = readFileSync(inputFile);
  const kind = format ?? detectFormat(bytes);
  if (kind === 'flat' && base === undefined && segments === undefined && givenOptions === undefined) {
    throw new FcdevError('平铺二进制需要指定加载地址或段列表', { code: 'ARGUMENT_MISSING', hint: '用 --base <十六进制地址> 指定，如 --base 8000；有换页时给出段列表' });
  }
  const formatOptions = kind === 'flat' && (base !== undefined || segments !== undefined) ? (segments === undefined ? { address: base } : { segments }) : { ...givenOptions };
  const parsed = parseImage(kind, bytes, formatOptions);
  const space = createAddressSpace(parsed.layout);
  const warnings = [];

  const vectors = useVectors ? entriesFromVectors(space, parsed.vectors, parsed.vectorSegmentId) : { entries: [], skipped: [] };
  warnings.push(...vectors.skipped.map((item) => `${item.name} 向量指向 ${hex(item.address)}：${item.reason}`));
  const declared = useVectors ? (parsed.entries ?? []).map((item) => entryAtAddress(space, item.address, { source: 'vector', description: item.description })) : [];
  const manual = entries.map((item) => (typeof item === 'number'
    ? entryAtAddress(space, item, { source: 'manual', description: '手工指定' })
    : entryAtAddress(space, item.address, { source: 'manual', description: '手工指定', segmentId: item.segmentId })));
  const traced = traceFile === undefined ? { entries: [], skipped: [] } : entriesFromAddresses(space, parseTrace(readFileSync(traceFile, 'utf8')), { source: 'trace', description: '指令轨迹' });
  warnings.push(...skippedWarnings('指令轨迹', traced.skipped));
  const recorded = cdlFile === undefined ? [] : entriesFromCodeFlags(parsed.layout, parseCdl(readFileSync(cdlFile), mappedSize(parsed.layout)), { source: 'cdl', description: '代码数据日志' });
  const allEntries = uniqueEntries([...vectors.entries, ...declared, ...manual, ...traced.entries, ...recorded]);
  if (requireEntries && allEntries.length === 0) {
    throw new FcdevError('没有入口可供追踪代码', { code: 'ARGUMENT_MISSING', hint: '用 --entry <十六进制地址> 指定入口，或用 --cdl、--trace 提供运行记录' });
  }

  const sets = loadSymbolSets(projectDir, symbolSetFiles);
  let project = createProject({
    imagePath: toProjectPath(projectDir, path.resolve(inputFile)),
    bytes,
    format: kind,
    formatOptions,
    layout: parsed.layout,
    entries: allEntries,
    symbolSets: sets.map(({ name, path: stored }) => ({ name, path: stored })),
    profile,
  });
  if (symbolsFile !== undefined) {
    // 同一地址有多个名字时只用第一个
    const defines = [];
    for (const symbol of parseSymbols(readFileSync(symbolsFile, 'utf8'))) {
      if (!defines.some((item) => item.address === symbol.address)) defines.push(symbol);
    }
    project = applyEdits(project, defines.map(({ name, address }) => ({ op: 'defineSymbol', name, address }))).project;
  }
  return { project, bytes, parsed, sets, warnings };
}
