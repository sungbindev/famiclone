// ------------------------------------------------------------------------
// 名称：project-operations.mjs
// 说明：工程与映像的操作：open（新建或打开）、status（进度报告）、layout（查看或替换布局）、hex（十六进制视图）、find（搜索）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：参数与结果都是可序列化的普通对象；地址参数可以写成整数或 "$C000" 这样的十六进制文字。
// ------------------------------------------------------------------------
import { existsSync } from 'node:fs';
import path from 'node:path';
import { FcdevError, readTargetProfile, selectProfileInput } from '../../shared/index.mjs';
import { scanCode } from '../analysis/index.mjs';
import { FORMAT_NAMES } from '../formats/index.mjs';
import { findUncovered, listRegions, regionStats, resolveSegmentId } from '../model/index.mjs';
import { findBytes, findInstructions, parseBytePattern, replaceLayout, textToPattern } from '../query/index.mjs';
import { buildStatus } from '../verify/index.mjs';
import { PROJECT_FILE, openImageProject, writeProject } from '../workspace/index.mjs';
import { loadContext, saveContext } from './context.mjs';
import { invalidParam, optionalAddress, optionalArray, optionalBoolean, optionalEnum, optionalInteger, optionalString, projectDirectory, requireString, toAddress } from './params.mjs';
import { renderHexView, buildHexRows } from '../view/index.mjs';

/** 十六进制视图字节数的上限 */
const MAX_HEX_LENGTH = 4096;

/**
 * 工程的概况：映像、格式、布局、数量与进度。
 * @param {import('./context.mjs').Context} context 上下文
 * @returns {object} 概况
 */
export function summarize(context) {
  const { project, bytes } = context;
  return {
    projectDir: context.dir,
    image: { path: project.image.path, size: project.image.size },
    format: project.format.name,
    profile: project.profile,
    segments: project.layout.segments.map((segment) => ({ id: segment.id, fileOffset: segment.fileOffset, length: segment.length, address: segment.address, bank: segment.bank, writable: segment.writable })),
    counts: {
      entries: project.entries.length, symbols: project.symbols.length, comments: project.comments.length, hints: project.hints.length, modules: project.modules.length, symbolSets: project.symbolSets.length,
    },
    symbolSets: project.symbolSets.map((set) => set.name),
    status: buildStatus(project, bytes),
  };
}

/**
 * 新建或打开工程。
 * @param {Record<string, unknown>} params projectDir；新建时再给 image，以及 target（内置目标描述的名字，或相对工程目录的 profile.json 路径）与 input、format、address 或 segments、entries、symbolSets、vectors、overwrite
 * @returns {object} 工程概况；新建时带 created 与警告
 */
export function open(params) {
  const dir = projectDirectory(params);
  const image = optionalString(params, 'image');
  if (image === undefined) return { created: false, ...summarize(loadContext(dir)) };

  if (existsSync(path.join(dir, PROJECT_FILE)) && !optionalBoolean(params, 'overwrite', false)) {
    throw new FcdevError(`${dir} 里已经有工程`, { code: 'DISASM_PROJECT_EXISTS', hint: '不给 image 就是打开它；要重新创建，加 overwrite: true' });
  }
  const entries = (optionalArray(params, 'entries') ?? []).map((item) => {
    if (item !== null && typeof item === 'object') return { address: toAddress(item.address, 'entries 里的 address'), segmentId: item.segmentId };
    return toAddress(item, 'entries');
  });
  const symbolSetFiles = (optionalArray(params, 'symbolSets') ?? []).map((file) => {
    if (typeof file !== 'string' || file === '') invalidParam('symbolSets 里必须是文件路径');
    return path.resolve(file);
  });
  const explicitFormat = optionalEnum(params, 'format', [...FORMAT_NAMES]);
  const explicitLayout = optionalAddress(params, 'address') !== undefined || optionalArray(params, 'segments') !== undefined;
  let format = explicitFormat;
  let formatOptions;
  let profile;
  const target = optionalString(params, 'target');
  if (target !== undefined) {
    const { profile: description } = readTargetProfile(target, { cwd: dir });
    const chosen = selectProfileInput(description, optionalString(params, 'input'));
    if (!explicitLayout && (explicitFormat === undefined || explicitFormat === chosen.format)) {
      format = chosen.format;
      formatOptions = { ...chosen.formatOptions };
    }
    symbolSetFiles.unshift(...description.symbolSets.filter((file) => !symbolSetFiles.includes(file)));
    profile = { name: description.name, input: chosen.name, graphicsPorts: [...description.graphicsPorts] };
  } else if (optionalString(params, 'input') !== undefined) {
    invalidParam('给了 input 就要同时给 target');
  }
  const opened = openImageProject({
    inputFile: path.resolve(image),
    projectDir: dir,
    format,
    formatOptions,
    base: optionalAddress(params, 'address'),
    segments: optionalArray(params, 'segments'),
    entries,
    symbolSetFiles,
    profile,
    useVectors: optionalBoolean(params, 'vectors', true),
  });
  writeProject(dir, opened.project);
  return { created: true, description: opened.parsed.description, warnings: opened.warnings, ...summarize(loadContext(dir)) };
}

/**
 * 进度报告。
 * @param {Record<string, unknown>} params projectDir
 * @returns {object} 工程概况
 */
export function status(params) {
  return summarize(loadContext(projectDirectory(params)));
}

/**
 * 查看或替换布局。
 * @param {Record<string, unknown>} params projectDir；替换时给 segments（完整的段列表）
 * @returns {object} 布局、没有划入任何段的字节范围；替换时带各项丢弃的个数
 */
export function layout(params) {
  const context = loadContext(projectDirectory(params));
  const segments = optionalArray(params, 'segments');
  let current = context;
  let dropped;
  if (segments !== undefined) {
    const replaced = replaceLayout(context.project, segments);
    current = saveContext(context, replaced.project);
    dropped = replaced.dropped;
  }
  const { project } = current;
  return {
    changed: segments !== undefined,
    segments: project.layout.segments.map((segment) => ({ ...segment })),
    uncovered: findUncovered(project.layout),
    regionBytes: regionStats(project.regions),
    ...(dropped === undefined ? {} : { dropped }),
  };
}

/**
 * 十六进制与字符视图。
 * @param {Record<string, unknown>} params projectDir、address；可选 segmentId、length、bytesPerRow
 * @returns {object} 文字视图与范围内的区域
 */
export function hex(params) {
  const context = loadContext(projectDirectory(params));
  const { project, bytes } = context;
  const address = optionalAddress(params, 'address');
  const requested = optionalInteger(params, 'segmentId');
  const segmentId = address === undefined ? (requested ?? project.layout.segments[0]?.id) : resolveSegmentId(project.layout, address, requested, 'DISASM_PARAM_INVALID');
  if (segmentId === null || segmentId === undefined) invalidParam('这个地址不在布局里', '先用 layout 查看布局');
  const length = optionalInteger(params, 'length', { min: 1, max: MAX_HEX_LENGTH }) ?? 256;
  const bytesPerRow = optionalInteger(params, 'bytesPerRow', { min: 1, max: 64 });
  const labels = new Map(project.symbols.filter((symbol) => symbol.segmentId === segmentId).map((symbol) => [symbol.address, symbol.name]));
  const options = { bytes, layout: project.layout, segmentId, address, length, bytesPerRow, labels };
  const { segment, start, end } = buildHexRows(options);
  const regions = listRegions(project.regions, segment.fileOffset + (start - segment.address), end - start).map((region) => ({
    address: segment.address + (region.offset - segment.fileOffset), length: region.length, type: region.type, source: region.source,
  }));
  return { segmentId, start, end, text: renderHexView(options), regions };
}

/**
 * 搜索字节、文本或指令。
 * @param {Record<string, unknown>} params projectDir、kind（bytes、text、instructions）、pattern 或 text；可选 encoding、limit、cursor
 * @returns {object} 匹配与下一页游标
 */
export function find(params) {
  const context = loadContext(projectDirectory(params));
  const kind = optionalEnum(params, 'kind', ['bytes', 'text', 'instructions'], 'bytes');
  const page = { limit: optionalInteger(params, 'limit', { min: 1 }), cursor: optionalString(params, 'cursor') };
  if (kind === 'instructions') return { kind, ...findInstructions({ scan: scanCode(context.project, context.bytes), pattern: requireString(params, 'pattern'), ...page }) };
  const pattern = kind === 'text'
    ? textToPattern(requireString(params, 'text'), optionalEnum(params, 'encoding', ['ascii', 'ascii-high'], 'ascii'))
    : parseBytePattern(requireString(params, 'pattern'));
  const result = findBytes({ project: context.project, bytes: context.bytes, pattern, ...page });
  return { kind, ...result, matches: result.matches.map((match) => ({ ...match, symbol: context.index.at(match.segmentId, match.address)?.name ?? null })) };
}
