/*
 * ------------------------------------------------------------------------
 * 名称：preferences-model.ts
 * 说明：首选项页面的纯逻辑：页面里显示哪些分组与设置项、取值是否与出厂默认不同、修改后的设置项列表、整数输入的解析与范围校验、错误说明的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：不依赖界面与请求，便于单独测试；规则见 docs/05-界面.md 第 4.5 节。
 * ------------------------------------------------------------------------
 */

import type { MachineInfo } from '../../contracts/machines/generated/machines';
import type { PreferenceGroup, PreferenceSetting } from '../../contracts/preferences/generated/preferences';

/** 用户可见的文字。 */
const TEXT = {
  notInteger: '请输入整数。',
  outOfRange: (min: number, max: number) => `请输入 ${min} 至 ${max} 之间的整数。`,
  failed: '操作失败。',
  machinesGroup: '机型与 BIOS',
  noBios: '未添加',
  unsupported: '暂未支持',
  biosFile: (name: string, kilobytes: number) => `${name}（${kilobytes} KB）`
} as const;

/** “机型与 BIOS”分组的标识：这个分组不是设置项，内容由页面自己提供（BIOS 的添加、替换与移除）。 */
export const MACHINES_GROUP_ID = 'machines';

/** 在声音分组之后插入“机型与 BIOS”分组（没有声音分组时放在最后），其余分组保持核心给出的顺序。 */
export function withMachinesGroup(groups: readonly PreferenceGroup[]): PreferenceGroup[] {
  const machines: PreferenceGroup = { id: MACHINES_GROUP_ID, label: TEXT.machinesGroup };
  const soundIndex = groups.findIndex((group) => group.id === 'sound');
  if (soundIndex < 0) return [...groups, machines];
  return [...groups.slice(0, soundIndex + 1), machines, ...groups.slice(soundIndex + 1)];
}

/** 一个机型的 BIOS 状态文字：暂未支持、未添加，或文件名与大小（KB，四舍五入）。 */
export function biosStatusText(machine: MachineInfo): string {
  if (!machine.supported) return TEXT.unsupported;
  if (machine.bios === undefined) return TEXT.noBios;
  return TEXT.biosFile(machine.bios.fileName, Math.round(machine.bios.size / 1024));
}

/** 整数输入的解析结果。 */
export type IntegerParse = { readonly ok: true; readonly value: number } | { readonly ok: false; readonly message: string };

/**
 * 页面里显示的设置项：隐藏的（只保存取值，如音量）不显示。
 * @param settings 全部设置项。
 */
export function visibleSettings(settings: readonly PreferenceSetting[]): PreferenceSetting[] {
  return settings.filter((setting) => !setting.hidden);
}

/**
 * 页面左侧显示的分组：至少有一个显示的设置项的分组，保持核心给出的顺序。
 * @param groups 全部分组。
 * @param settings 全部设置项。
 */
export function visibleGroups(groups: readonly PreferenceGroup[], settings: readonly PreferenceSetting[]): PreferenceGroup[] {
  const shown = new Set(visibleSettings(settings).map((setting) => setting.group));
  return groups.filter((group) => shown.has(group.id));
}

/**
 * 当前取值是否与出厂默认值不同。
 * @param setting 设置项。
 */
export function isChanged(setting: PreferenceSetting): boolean {
  return !Object.is(setting.value, setting.defaultValue);
}

/**
 * 取值变化之后的设置项列表；不认识的标识原样返回。
 * @param settings 全部设置项。
 * @param id 变化的设置项标识。
 * @param value 新的取值。
 */
export function withValue(settings: readonly PreferenceSetting[], id: string, value: unknown): PreferenceSetting[] {
  return settings.map((setting) => (setting.id === id ? { ...setting, value } : setting));
}

/**
 * 解析整数输入并按设置项登记的范围校验。
 * @param text 输入的文字。
 * @param setting 设置项。
 */
export function parseInteger(text: string, setting: Pick<PreferenceSetting, 'minimum' | 'maximum'>): IntegerParse {
  const trimmed = text.trim();
  if (!/^-?\d+$/.test(trimmed)) return { ok: false, message: TEXT.notInteger };
  const value = Number(trimmed);
  const { minimum, maximum } = setting;
  if ((minimum !== undefined && value < minimum) || (maximum !== undefined && value > maximum)) {
    return { ok: false, message: TEXT.outOfRange(minimum ?? Number.MIN_SAFE_INTEGER, maximum ?? Number.MAX_SAFE_INTEGER) };
  }
  return { ok: true, value };
}

/**
 * 从请求失败的原因里取出给用户看的说明。
 * @param error 请求失败时抛出的值（核心的错误为 { code, message }）。
 */
export function errorText(error: unknown): string {
  if (typeof error === 'object' && error !== null && 'message' in error && typeof error.message === 'string' && error.message !== '') return error.message;
  return TEXT.failed;
}
