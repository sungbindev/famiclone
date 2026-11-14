/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges.ts
 * 说明：最近打开的卡带清单的规则：最多保留的个数、从保存的内容里读出清单（逐项校验，损坏时得到空清单）、加入与移出、取显示名、转成要保存的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：清单只存卡带文件的路径，由主进程使用，渲染进程只能拿到显示名；规则见 docs/05-界面.md 第 5 节与 docs/04-调试服务.md 的 cartridges.recent。
 * ------------------------------------------------------------------------
 */

/** 最近打开的卡带最多保留的个数。 */
export const MAX_RECENT_CARTRIDGES = 10;
/** 保存内容的版本号。 */
export const RECENT_CARTRIDGES_VERSION = 1;
/** 一条路径的长度上限：超过的视为损坏的内容。 */
const MAX_PATH_LENGTH = 4096;
/** 卡带文件的扩展名。 */
const CARTRIDGE_EXTENSION = '.nes';

/** 保存在文件里的内容。 */
export interface RecentCartridgesFile {
  readonly version: number;
  readonly items: ReadonlyArray<{ readonly path: string }>;
}

/** 路径合格：非空、不过长、是 .nes 文件。 */
function isValidPath(value: unknown): value is string {
  return typeof value === 'string' && value.length > 0 && value.length <= MAX_PATH_LENGTH && value.toLowerCase().endsWith(CARTRIDGE_EXTENSION);
}

/**
 * 从保存的内容里读出清单：只接受结构合格的项，去掉重复，超出个数的丢弃；内容缺失或损坏时返回空清单。
 * @param raw 读到的内容。
 * @returns 路径清单，最近的在前。
 */
export function parseRecentCartridges(raw: unknown): readonly string[] {
  if (typeof raw !== 'object' || raw === null || Array.isArray(raw)) return [];
  const items = (raw as { items?: unknown }).items;
  if (!Array.isArray(items)) return [];
  const result: string[] = [];
  for (const item of items) {
    const path = typeof item === 'object' && item !== null ? (item as { path?: unknown }).path : undefined;
    if (isValidPath(path) && !result.includes(path)) result.push(path);
    if (result.length >= MAX_RECENT_CARTRIDGES) break;
  }
  return result;
}

/**
 * 把一个卡带加到清单最前面；已在清单里的移到最前，超出个数的丢弃最早的。
 * @param list 当前清单。
 * @param path 卡带路径。
 * @returns 新清单；不是 .nes 路径时原样返回。
 */
export function addRecentCartridge(list: readonly string[], path: string): readonly string[] {
  if (!isValidPath(path)) return list;
  return [path, ...list.filter((entry) => entry !== path)].slice(0, MAX_RECENT_CARTRIDGES);
}

/**
 * 把一个卡带移出清单。
 * @param list 当前清单。
 * @param path 卡带路径。
 * @returns 新清单；不在清单里时原样返回。
 */
export function removeRecentCartridge(list: readonly string[], path: string): readonly string[] {
  return list.includes(path) ? list.filter((entry) => entry !== path) : list;
}

/**
 * 路径里的文件名，作为界面上的显示名。
 * @param path 卡带路径。
 */
export function recentCartridgeName(path: string): string {
  return path.split(/[\\/]/).pop() ?? path;
}

/**
 * 转成要保存的内容。
 * @param list 清单。
 */
export function toRecentCartridgesFile(list: readonly string[]): RecentCartridgesFile {
  return { version: RECENT_CARTRIDGES_VERSION, items: list.map((path) => ({ path })) };
}
