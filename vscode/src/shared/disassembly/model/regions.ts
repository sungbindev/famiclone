// ------------------------------------------------------------------------
// 名称：regions.ts
// 说明：区域模型：按文件偏移给映像里的字节标注类型（代码、字节表、地址表、字符串、图形、填充），没
//       有标注的字节就是 unknown。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：区域互不重叠，设置新区域时盖住的旧区域被裁剪或拆开；区域必须落在同一个段里；地址表（words）
//       的长度是偶数，新区域只能在表项边界上切开它，否则报冲突。不同类型的参数各有固定的键，多余的
//       键报错。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';
import { locateOffset } from './layout';
import type { Layout } from './layout';

/** 区域类型，unknown 是没有标注的状态，不会存成区域 */
export const REGION_TYPES = Object.freeze(['code', 'bytes', 'words', 'string', 'graphics', 'padding', 'unknown']);

export interface Region {
  /** 第一个字节在映像文件里的偏移 */
  offset: number;
  /** 字节数 */
  length: number;
  /** 区域类型，见 REGION_TYPES */
  type: string;
  /** 类型的参数 */
  params: Record<string, unknown>;
  /** 标注的来源，如 trace、table、manual、detect */
  source: string;
}

export interface RegionMap {
  /** 布局 */
  layout: Layout;
  /** 按偏移排序、互不重叠的区域 */
  regions: Region[];
}

/** 各类型允许的参数：键到检查函数 */
const PARAM_RULES: Record<string, Record<string, (value: unknown) => boolean>> = {
  code: {},
  bytes: { perLine: (value) => isIntegerIn(value, 1, 32), role: (value) => typeof value === 'string' },
  words: { perLine: (value) => isIntegerIn(value, 1, 16), adjust: (value) => value === 0 || value === 1 },
  string: { encoding: (value) => typeof value === 'string', terminator: (value) => value === null || value === 0 || value === 'high-bit' },
  graphics: { format: (value) => typeof value === 'string' },
  padding: { value: (value) => isIntegerIn(value, 0, 255) },
};

/**
 * 判断值是不是指定范围内的整数。
 * @param value 值
 * @param min 最小值
 * @param max 最大值
 * @returns 是整数且在范围内时为 true
 */
function isIntegerIn(value: unknown, min: number, max: number): boolean {
  return typeof value === 'number' && Number.isInteger(value) && value >= min && value <= max;
}

/**
 * 抛出区域错误。
 * @param code 错误码
 * @param message 说明
 * @returns
 */
function fail(code: string, message: string): never {
  throw new FcdevError(message, { code });
}

/**
 * 创建空的区域表。
 * @param layout 布局
 * @returns 区域表
 */
export function createRegionMap(layout: Layout): RegionMap {
  return { layout, regions: [] };
}

/**
 * 检查并规范化区域的参数。
 * @param type 区域类型
 * @param params 参数
 * @returns 参数的副本
 */
function normalizeParams(type: string, params: Record<string, unknown>|undefined = {}): Record<string, unknown> {
  const rules = PARAM_RULES[type] ?? {};
  for (const [key, value] of Object.entries(params)) {
    if (!Object.hasOwn(rules, key)) fail('DISASM_REGION_INVALID', `${type} 区域没有参数 ${key}`);
    if (!rules[key]!(value)) fail('DISASM_REGION_INVALID', `${type} 区域的参数 ${key} 取值不合法：${String(value)}`);
  }
  return { ...params };
}

/**
 * 检查区域落在同一个段里。
 * @param map 区域表
 * @param offset 起点
 * @param length 字节数
 */
function assertInsideOneSegment(map: RegionMap, offset: number, length: number) {
  if (!Number.isInteger(offset) || !Number.isInteger(length) || length < 1) fail('DISASM_REGION_INVALID', '区域的偏移必须是整数，长度必须是正整数');
  const first = locateOffset(map.layout, offset);
  const last = locateOffset(map.layout, offset + length - 1);
  if (first === null || last === null || first.segment.id !== last.segment.id) {
    fail('DISASM_REGION_INVALID', `区域（偏移 ${offset}，长度 ${length}）必须完整地落在某一个段里`);
  }
}

/**
 * 把区域 [start, end) 从已有的区域里挖掉，两头不被盖住的部分保留。
 * @param map 区域表
 * @param start 起点
 * @param end 终点（不含）
 * @returns 挖掉之后剩下的区域，未排序
 * @throws FcdevError 切开地址表时不在表项边界上
 */
function carve(map: RegionMap, start: number, end: number): Region[] {
  const kept: Region[] = [];
  for (const region of map.regions) {
    const regionEnd = region.offset + region.length;
    if (regionEnd <= start || region.offset >= end) {
      kept.push(region);
      continue;
    }
    const leftLength = Math.max(0, start - region.offset);
    const rightLength = Math.max(0, regionEnd - end);
    if (region.type === 'words' && ((leftLength > 0 && leftLength % 2 !== 0) || (rightLength > 0 && (regionEnd - rightLength - region.offset) % 2 !== 0))) {
      fail('DISASM_REGION_CONFLICT', `新区域不在地址表（偏移 ${region.offset}）的表项边界上，不能切开它`);
    }
    if (leftLength > 0) kept.push({ ...region, length: leftLength });
    if (rightLength > 0) kept.push({ ...region, offset: end, length: rightLength });
  }
  return kept;
}

/**
 * 设置一个区域，盖住的旧区域被裁剪或拆开；类型为 unknown 时等于清除。
 * @param map 区域表，会被修改
 * @param input 区域
 * @returns 新增的区域；清除时为 null
 * @throws FcdevError 类型或参数不合法、区域跨段、地址表长度不是偶数、切开地址表时不在表项边界上
 */
export function setRegion(map: RegionMap, { offset, length, type, params, source }: {offset: number, length: number, type: string, params?: Record<string, unknown>, source: string}): Region|null {
  if (!REGION_TYPES.includes(type)) fail('DISASM_REGION_INVALID', `未知的区域类型：${type}`);
  assertInsideOneSegment(map, offset, length);
  if (typeof source !== 'string' || source === '') fail('DISASM_REGION_INVALID', '区域要写明标注的来源');
  if (type === 'words' && length % 2 !== 0) fail('DISASM_REGION_INVALID', '地址表的长度必须是偶数');
  const normalized = type === 'unknown' ? null : { offset, length, type, params: normalizeParams(type, params), source };
  const kept = carve(map, offset, offset + length);
  if (normalized !== null) kept.push(normalized);
  map.regions = kept.toSorted((a, b) => a.offset - b.offset);
  return normalized;
}

/**
 * 取包含某个偏移的区域。
 * @param map 区域表
 * @param offset 文件偏移
 * @returns 没有标注（unknown）时为 null
 */
export function regionAt(map: RegionMap, offset: number): Region|null {
  return map.regions.find((region) => offset >= region.offset && offset < region.offset + region.length) ?? null;
}

/**
 * 列出一段范围内的区域，没有标注的部分补成 unknown 区域。
 * @param map 区域表
 * @param offset 起点
 * @param length 字节数
 * @returns 按偏移排序、首尾相接地覆盖整个范围；区域被范围截断的部分按范围裁剪
 */
export function listRegions(map: RegionMap, offset: number, length: number): Region[] {
  const end = offset + length;
  const result: Region[] = [];
  let cursor = offset;
  for (const region of map.regions) {
    const regionEnd = region.offset + region.length;
    if (regionEnd <= offset || region.offset >= end) continue;
    const from = Math.max(region.offset, offset);
    const to = Math.min(regionEnd, end);
    if (from > cursor) result.push({ offset: cursor, length: from - cursor, type: 'unknown', params: {}, source: 'none' });
    result.push({ ...region, offset: from, length: to - from });
    cursor = to;
  }
  if (cursor < end) result.push({ offset: cursor, length: end - cursor, type: 'unknown', params: {}, source: 'none' });
  return result;
}

/**
 * 统计各类型区域占的字节数；没有标注的字节算在 unknown 里，只统计段内的字节。
 * @param map 区域表
 * @returns 类型到字节数，REGION_TYPES 里的每个类型都有
 */
export function regionStats(map: RegionMap): Record<string, number> {
  const stats = Object.fromEntries(REGION_TYPES.map((type) => [type, 0]));
  let mapped = 0;
  for (const segment of map.layout.segments) mapped += segment.length;
  let covered = 0;
  for (const region of map.regions) {
    stats[region.type] = (stats[region.type] ?? 0) + region.length;
    covered += region.length;
  }
  stats.unknown = mapped - covered;
  return stats;
}
