// ------------------------------------------------------------------------
// 名称：project.mjs
// 说明：反汇编工程：映像的摘要、格式、布局、入口、区域，以及符号、注释、操作数提示、模块、符号集；可存成 disasm.json，也可从中读回。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：工程文件是文本 JSON，字段顺序固定，便于用 git 比较；工程是数据，不含映像字节（映像按 image.path 读取，摘要不一致说明工程已过期）。
//       unresolved 是最近一次追踪留下的待处理项，verification 是最近一次回读的结果；这两项不参与“生成输入”的摘要，
//       摘要变化说明生成结果可能变了，回读结果因此过期。
// ------------------------------------------------------------------------
import { createHash } from 'node:crypto';
import { FcdevError } from '../../shared/index.mjs';
import { createEntry } from './entries.mjs';
import { createLayout } from './layout.mjs';
import { createRegionMap, setRegion } from './regions.mjs';
import { assertModulesDisjoint, normalizeComment, normalizeHint, normalizeModule, normalizeSymbol } from './symbols.mjs';

/** 工程文件的版本号 */
export const PROJECT_VERSION = 1;

/** 符号集文件名：放进工程 include 目录的 .inc 文件 */
const SYMBOL_SET_NAME = /^[A-Za-z0-9_-]+\.inc$/;

/**
 * 抛出工程错误。
 * @param {string} message 说明
 * @returns {never}
 */
function invalid(message) {
  throw new FcdevError(message, { code: 'DISASM_PROJECT_INVALID' });
}

/**
 * 计算字节的 SHA-256。
 * @param {Uint8Array} bytes 字节
 * @returns {string} 十六进制摘要
 */
export function sha256(bytes) {
  return createHash('sha256').update(bytes).digest('hex');
}

/**
 * @typedef {object} Project
 * @property {number} version 工程文件版本
 * @property {{path: string, size: number, sha256: string}} image 映像文件的路径、大小与摘要
 * @property {{name: string, options: Record<string, unknown>}} format 映像格式与解析选项
 * @property {{name: string, input: string, graphicsPorts: number[]}|null} profile 创建工程时用的目标描述：名字、输入名与图形数据端口；没有时为 null
 * @property {import('./layout.mjs').Layout} layout 布局
 * @property {Array<Readonly<import('./entries.mjs').Entry>>} entries 入口
 * @property {import('./regions.mjs').RegionMap} regions 区域表
 * @property {Array<ReturnType<typeof normalizeSymbol>>} symbols 符号
 * @property {Array<ReturnType<typeof normalizeComment>>} comments 注释
 * @property {Array<ReturnType<typeof normalizeHint>>} hints 操作数提示
 * @property {Array<ReturnType<typeof normalizeModule>>} modules 模块
 * @property {Array<{name: string, path: string}>} symbolSets 符号集：include 目录里的文件名与来源路径
 * @property {Array<Record<string, unknown>>} unresolved 最近一次追踪留下的未解析间接跳转与跨 bank 转移
 * @property {{ok: boolean, digest: string, at: string, firstDifference?: Record<string, unknown>}|null} verification 最近一次回读的结果
 */

/**
 * 检查符号集的文件名与路径。
 * @param {{name: string, path: string}} set 符号集
 * @returns {{name: string, path: string}} 符号集
 */
function normalizeSymbolSet(set) {
  if (typeof set?.name !== 'string' || !SYMBOL_SET_NAME.test(set.name)) invalid(`符号集的文件名不合法：${String(set?.name)}（应为 xxx.inc）`);
  if (typeof set.path !== 'string' || set.path === '') invalid(`符号集 ${set.name} 要给出来源路径`);
  return { name: set.name, path: set.path };
}

/**
 * 检查目标描述的记录。
 * @param {unknown} profile 目标描述的记录；没有时为 null 或 undefined
 * @returns {{name: string, input: string, graphicsPorts: number[]}|null} 规范化的记录
 */
function normalizeProfileRecord(profile) {
  if (profile === null || profile === undefined) return null;
  const ports = profile.graphicsPorts;
  if (typeof profile.name !== 'string' || typeof profile.input !== 'string' || !Array.isArray(ports) || !ports.every((port) => Number.isInteger(port) && port >= 0 && port <= 0xffff)) {
    invalid('工程文件的 profile 不完整');
  }
  return { name: profile.name, input: profile.input, graphicsPorts: [...ports] };
}

/**
 * 创建工程。
 * @param {{imagePath: string, bytes: Uint8Array, format: string, formatOptions?: Record<string, unknown>, layout: import('./layout.mjs').Layout, entries?: Array<Readonly<import('./entries.mjs').Entry>>, symbolSets?: Array<{name: string, path: string}>, profile?: {name: string, input: string, graphicsPorts: number[]}}} input 映像路径与字节、格式、布局、入口、符号集、目标描述的记录
 * @returns {Project} 工程
 */
export function createProject({ imagePath, bytes, format, formatOptions = {}, layout, entries = [], symbolSets = [], profile }) {
  if (layout.imageSize !== bytes.length) invalid(`布局的映像大小（${layout.imageSize}）与映像字节数（${bytes.length}）不一致`);
  return {
    version: PROJECT_VERSION,
    image: { path: imagePath, size: bytes.length, sha256: sha256(bytes) },
    format: { name: format, options: { ...formatOptions } },
    profile: normalizeProfileRecord(profile),
    layout,
    entries: [...entries],
    regions: createRegionMap(layout),
    symbols: [],
    comments: [],
    hints: [],
    modules: [],
    symbolSets: symbolSets.map(normalizeSymbolSet),
    unresolved: [],
    verification: null,
  };
}

/**
 * 序列化成 disasm.json 的文本。
 * @param {Project} project 工程
 * @returns {string} 带结尾换行的 JSON
 */
export function serializeProject(project) {
  return `${JSON.stringify({
    version: project.version,
    image: project.image,
    format: project.format,
    profile: project.profile,
    layout: { imageSize: project.layout.imageSize, segments: project.layout.segments },
    entries: project.entries,
    regions: project.regions.regions,
    symbols: project.symbols,
    comments: project.comments,
    hints: project.hints,
    modules: project.modules,
    symbolSets: project.symbolSets,
    unresolved: project.unresolved,
    verification: project.verification,
  }, null, 2)}\n`;
}

/**
 * 从 disasm.json 的文本读回工程，逐项检查。
 * @param {string} text 文件内容
 * @returns {Project} 工程
 * @throws {FcdevError} 不是合法的 JSON、版本不对，或任何一项不合法
 */
export function parseProject(text) {
  let data;
  try {
    data = JSON.parse(text);
  } catch (error) {
    throw new FcdevError('工程文件不是合法的 JSON', { code: 'DISASM_PROJECT_INVALID', cause: error });
  }
  if (data === null || typeof data !== 'object' || data.version !== PROJECT_VERSION) invalid(`不支持的工程文件版本：${String(data?.version)}`);
  for (const key of ['entries', 'regions', 'symbols', 'comments', 'hints', 'modules', 'symbolSets', 'unresolved']) {
    if (!Array.isArray(data[key])) invalid(`工程文件缺少 ${key}`);
  }
  if (typeof data.image?.path !== 'string' || !Number.isInteger(data.image.size) || typeof data.image.sha256 !== 'string') invalid('工程文件的 image 不完整');
  if (typeof data.format?.name !== 'string' || typeof data.format.options !== 'object' || data.format.options === null) invalid('工程文件的 format 不完整');
  const layout = createLayout(data.layout?.segments ?? [], { imageSize: data.layout?.imageSize });
  const regions = createRegionMap(layout);
  for (const region of data.regions) setRegion(regions, region);
  const modules = data.modules.map((module) => normalizeModule(layout, module));
  assertModulesDisjoint(modules);
  const symbols = data.symbols.map((symbol) => normalizeSymbol(layout, symbol));
  const names = new Set();
  for (const symbol of symbols) {
    if (names.has(symbol.name)) throw new FcdevError(`符号 ${symbol.name} 重复`, { code: 'DISASM_SYMBOL_INVALID' });
    names.add(symbol.name);
  }
  return {
    version: PROJECT_VERSION,
    image: { path: data.image.path, size: data.image.size, sha256: data.image.sha256 },
    format: { name: data.format.name, options: { ...data.format.options } },
    profile: normalizeProfileRecord(data.profile),
    layout,
    entries: data.entries.map((entry) => createEntry(layout, entry)),
    regions,
    symbols,
    comments: data.comments.map((comment) => normalizeComment(layout, comment)),
    hints: data.hints.map((hint) => normalizeHint(layout, hint)),
    modules,
    symbolSets: data.symbolSets.map(normalizeSymbolSet),
    unresolved: data.unresolved,
    verification: data.verification ?? null,
  };
}

/**
 * 复制工程：和原工程互不影响。
 * @param {Project} project 工程
 * @returns {Project} 副本
 */
export function cloneProject(project) {
  return parseProject(serializeProject(project));
}

/**
 * 生成输入的摘要：布局、入口、区域、符号、注释、提示、模块、符号集与格式；变化说明生成结果可能变了。
 * @param {Project} project 工程
 * @returns {string} 十六进制摘要
 */
export function projectDigest(project) {
  const text = JSON.stringify([project.image.sha256, project.format, project.layout.segments, project.entries, project.regions.regions, project.symbols, project.comments, project.hints, project.modules, project.symbolSets]);
  return createHash('sha256').update(text).digest('hex');
}

/**
 * 映像文件与工程记录的摘要是否不一致。
 * @param {Project} project 工程
 * @param {Uint8Array} bytes 映像字节
 * @returns {boolean} 不一致（工程已过期）时为 true
 */
export function isImageStale(project, bytes) {
  return bytes.length !== project.image.size || sha256(bytes) !== project.image.sha256;
}

/**
 * 最近一次回读的结果是否已过期：没有回读过，或回读之后工程又改过。
 * @param {Project} project 工程
 * @returns {boolean} 过期时为 true
 */
export function isVerificationStale(project) {
  return project.verification === null || project.verification.digest !== projectDigest(project);
}
