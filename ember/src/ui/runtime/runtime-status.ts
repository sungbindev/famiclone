/*
 * ------------------------------------------------------------------------
 * 名称：runtime-status.ts
 * 说明：运行模块用到的纯规则：从 runtime.status 的结果得出机器的状态（未开机、已断电、已暂停、运行中）、各控制是否可用，以及运行面板与状态栏显示的文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：可用性规则见 docs/05-界面.md 第 4.1 节：没有开机时暂停、复位置灰；断电时暂停与软复位置灰；硬复位只在通电时可用；
 *       “开机”按已选机型判断（见 powerToggleAction）；“关闭卡带”只在装着卡带时可用。状态还没读到（核心启动中、重启中或不可用）时为 unknown，全部置灰。
 *       卡带、mapper 插件、电源状态没有信息时文字函数返回 undefined，界面不显示这一项，不用“—”或“无”占位；主机型号总有值（未开机时是已选机型），只在状态还没读到时没有。
 * ------------------------------------------------------------------------
 */

import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';

/** 机器的状态：unknown 为还没读到，none 为没有开机，其余是开机后的电源与运行模式。 */
export type MachineState = 'unknown' | 'none' | 'off' | 'paused' | 'running';

/** 用户可见的文字。 */
const TEXT = {
  none: '未开机',
  off: '已断电',
  paused: '已暂停',
  running: '运行中',
  powerOn: '开机',
  powerOff: '断电',
  pause: '暂停',
  resume: '继续'
} as const;

/** 主机型号的显示名；表里没有的型号显示型号标识本身。 */
const HOST_NAMES: Readonly<Record<string, string>> = { fc: '经典 FC', sb97: '小霸王 97', sb2000: '小霸王 2000', bbk10: '步步高 1.0', bbk20: '步步高 2.0' };

/** 各控制是否可用。 */
export interface ControlAvailability {
  /** 开机或断电。 */
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

/**
 * 是否捕获鼠标：设置打开，并且机器带鼠标——整机机型（学习机）有串口鼠标，经典 FC 只有卡带的 mapper 插件声明了 mouse 外设才有。
 * @param enabled 首选项“点击画面捕获鼠标”的取值。
 * @param status runtime.status 的结果；还没读到时为 undefined。
 */
export function capturesMouse(enabled: boolean, status: RuntimeStatusResult | undefined): boolean {
  if (!enabled || status?.machine === undefined) return false;
  if (status.machine !== 'fc') return true;
  return status.cartridge?.devices.includes('mouse') === true;
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

/** 状态文字；还没读到状态时没有信息，为 undefined。 */
export function stateText(status: RuntimeStatusResult | undefined): string | undefined {
  const state = machineState(status);
  return state === 'unknown' ? undefined : TEXT[state];
}

/**
 * 主机型号的显示文字：开机后是正在运行的主机，没有开机时是已选机型；状态还没读到时没有信息，为 undefined。
 * @param status runtime.status 的结果；还没读到时为 undefined。
 * @param selectedMachine 已选机型的标识。
 */
export function hostText(status: RuntimeStatusResult | undefined, selectedMachine: string): string | undefined {
  if (status === undefined) return undefined;
  const machine = status.machine ?? selectedMachine;
  return HOST_NAMES[machine] ?? machine;
}

/** 卡带的显示文字：卡带文件名；没有卡带（含整机机型用 BIOS 开机）时没有信息，为 undefined。 */
export function cartridgeText(status: RuntimeStatusResult | undefined): string | undefined {
  return status?.cartridge === undefined ? undefined : fileNameOf(status.cartridge.path);
}

/** mapper 插件的显示文字：插件名；没有卡带时没有信息，为 undefined。 */
export function pluginText(status: RuntimeStatusResult | undefined): string | undefined {
  return status?.cartridge?.pluginName;
}

/** 开机或断电按钮与菜单项的名称：通电时是“断电”，否则是“开机”。 */
export function powerToggleText(status: RuntimeStatusResult | undefined): string {
  return status?.power === 'on' ? TEXT.powerOff : TEXT.powerOn;
}

/** 暂停或继续按钮与菜单项的名称：连续运行时是“暂停”，否则是“继续”。 */
export function pauseToggleText(status: RuntimeStatusResult | undefined): string {
  return machineState(status) === 'running' ? TEXT.pause : TEXT.resume;
}

/** 机型清单里开机规则用到的部分：标识、是否已支持、已添加的 BIOS。 */
export interface BootableMachine {
  readonly id: string;
  readonly supported: boolean;
  readonly bios?: unknown;
}

/** 开机或断电按钮这次要做的事：断电、上电（已装入的主机）、用已选机型的 BIOS 开机，或不能做。 */
export type PowerToggleAction = 'powerOff' | 'powerOn' | 'boot' | 'none';

/**
 * 开机或断电按钮这次要做什么。通电时是断电；未通电时按已选机型：整机机型（学习机）已支持并已添加 BIOS 就开机（不需要卡带，当前装的是别的主机也用它重新开机），
 * 经典 FC 要求已经装入 FC 卡带，有就上电。
 * @param status runtime.status 的结果；还没读到时为 undefined，不能做。
 * @param selectedMachine 已选机型的标识。
 * @param machines 机型清单。
 */
export function powerToggleAction(status: RuntimeStatusResult | undefined, selectedMachine: string, machines: readonly BootableMachine[]): PowerToggleAction {
  if (status === undefined) return 'none';
  if (status.power === 'on') return 'powerOff';
  if (selectedMachine === 'fc') return status.machine === 'fc' ? 'powerOn' : 'none';
  const info = machines.find((item) => item.id === selectedMachine);
  return info !== undefined && info.supported && info.bios !== undefined ? 'boot' : 'none';
}

/**
 * 各控制是否可用。
 * @param status runtime.status 的结果；还没读到时为 undefined，全部置灰。
 * @param selectedMachine 已选机型的标识，决定“开机”是否可用。
 * @param machines 机型清单。
 */
export function controlAvailability(status: RuntimeStatusResult | undefined, selectedMachine: string, machines: readonly BootableMachine[]): ControlAvailability {
  const state = machineState(status);
  const powered = state === 'paused' || state === 'running';
  return { powerToggle: powerToggleAction(status, selectedMachine, machines) !== 'none', pauseToggle: powered, softReset: powered, hardReset: powered, eject: status?.cartridge !== undefined };
}
