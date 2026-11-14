/*
 * ------------------------------------------------------------------------
 * 名称：runtime-status.ts
 * 说明：运行页用到的纯规则：从 runtime.status 的结果得出机器的状态（未开机、已断电、已暂停、运行中）、各控制是否可用，以及边栏与状态栏显示的文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：可用性规则见 docs/05-界面.md 第 4.1 节：没有开机时全部置灰；断电时暂停与软复位置灰；硬复位只要已开机就可用；
 *       “关闭卡带”只在装着卡带时可用。状态还没读到（核心启动中、重启中或不可用）时为 unknown，显示“—”，全部置灰。
 * ------------------------------------------------------------------------
 */

import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';

/** 机器的状态：unknown 为还没读到，none 为没有开机，其余是开机后的电源与运行模式。 */
export type MachineState = 'unknown' | 'none' | 'off' | 'paused' | 'running';

/** 用户可见的文字。 */
const TEXT = {
  unknown: '—',
  none: '未开机',
  off: '已断电',
  paused: '已暂停',
  running: '运行中',
  noCartridge: '无',
  noCartridgeBios: '无（整机 BIOS）',
  noPlugin: '—',
  powerOn: '上电',
  powerOff: '断电',
  pause: '暂停',
  resume: '继续',
  muted: '已静音',
  volume: (value: number) => `音量 ${value}%`
} as const;

/** 主机型号的显示名；表里没有的型号显示型号标识本身。 */
const HOST_NAMES: Readonly<Record<string, string>> = { fc: '经典 FC', sb97: '小霸王 97', sb2000: '小霸王 2000', bbk10: '步步高 1.0', bbk20: '步步高 2.0' };

/** 各控制是否可用。 */
export interface ControlAvailability {
  /** 上电或断电。 */
  readonly powerToggle: boolean;
  /** 暂停或继续。 */
  readonly pauseToggle: boolean;
  /** 软复位。 */
  readonly softReset: boolean;
  /** 硬复位。 */
  readonly hardReset: boolean;
  /** 关闭（取出）卡带。 */
  readonly eject: boolean;
}

/** 路径里的文件名部分。 */
export function fileNameOf(path: string): string {
  return path.split(/[\\/]/).pop() ?? path;
}

/**
 * 机器的状态。
 * @param status runtime.status 的结果；还没读到时为 undefined。
 */
export function machineState(status: RuntimeStatusResult | undefined): MachineState {
  if (status === undefined) return 'unknown';
  if (status.machine === undefined) return 'none';
  if (status.power === 'off') return 'off';
  return status.mode === 'running' ? 'running' : 'paused';
}

/** 状态文字。 */
export function stateText(status: RuntimeStatusResult | undefined): string {
  return TEXT[machineState(status)];
}

/** 主机型号的显示文字；没有开机时为“—”。 */
export function hostText(status: RuntimeStatusResult | undefined): string {
  if (status?.machine === undefined) return TEXT.unknown;
  return HOST_NAMES[status.machine] ?? status.machine;
}

/** 卡带的显示文字：卡带文件名；没有卡带时为“无”，整机机型开机后（没有卡带、用 BIOS 开机）为“无（整机 BIOS）”。 */
export function cartridgeText(status: RuntimeStatusResult | undefined): string {
  if (status?.cartridge !== undefined) return fileNameOf(status.cartridge.path);
  return status?.machine !== undefined && status.machine !== 'fc' ? TEXT.noCartridgeBios : TEXT.noCartridge;
}

/** mapper 插件的显示文字：插件名；没有卡带时为“—”。 */
export function pluginText(status: RuntimeStatusResult | undefined): string {
  return status?.cartridge === undefined ? TEXT.noPlugin : status.cartridge.pluginName;
}

/** 上电或断电按钮与菜单项的名称：通电时是“断电”，否则是“上电”。 */
export function powerToggleText(status: RuntimeStatusResult | undefined): string {
  return status?.power === 'on' ? TEXT.powerOff : TEXT.powerOn;
}

/** 暂停或继续按钮与菜单项的名称：连续运行时是“暂停”，否则是“继续”。 */
export function pauseToggleText(status: RuntimeStatusResult | undefined): string {
  return machineState(status) === 'running' ? TEXT.pause : TEXT.resume;
}

/** 音量的显示文字：静音时是“已静音”。 */
export function volumeText(volume: number, muted: boolean): string {
  return muted ? TEXT.muted : TEXT.volume(volume);
}

/**
 * 各控制是否可用。
 * @param status runtime.status 的结果；还没读到时为 undefined，全部置灰。
 */
export function controlAvailability(status: RuntimeStatusResult | undefined): ControlAvailability {
  const state = machineState(status);
  const opened = state === 'off' || state === 'paused' || state === 'running';
  const powered = state === 'paused' || state === 'running';
  return { powerToggle: opened, pauseToggle: powered, softReset: powered, hardReset: opened, eject: status?.cartridge !== undefined };
}
