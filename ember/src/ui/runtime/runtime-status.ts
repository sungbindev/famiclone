/*
 * ------------------------------------------------------------------------
 * 名称：runtime-status.ts
 * 说明：运行页操作条用到的纯规则：从 runtime.status 的结果得出显示的状态文字与各按钮是否可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：按钮可用性规则见 docs/05-界面.md 第 4.1 节：没有卡带时除“打开卡带”外全部置灰；断电时暂停与软复位置灰；硬复位只要装入了卡带就可用。
 * ------------------------------------------------------------------------
 */

import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';

/** 用户可见的状态文字。 */
const TEXT = {
  noCartridge: '未装入卡带',
  powerOff: '已断电',
  paused: '已暂停',
  running: '运行中'
} as const;

/** 各按钮是否可用。 */
export interface ControlAvailability {
  /** 上电或断电。 */
  readonly powerToggle: boolean;
  /** 暂停或继续。 */
  readonly pauseToggle: boolean;
  /** 软复位。 */
  readonly softReset: boolean;
  /** 硬复位。 */
  readonly hardReset: boolean;
}

/** 路径里的文件名部分。 */
function fileName(path: string): string {
  return path.split(/[\\/]/).pop() ?? path;
}

/**
 * 状态文字：没有卡带时是“未装入卡带”，否则是电源与运行模式，后面跟卡带文件名。
 * @param status runtime.status 的结果；还没读到时为 undefined。
 */
export function statusText(status: RuntimeStatusResult | undefined): string {
  if (status === undefined) return '';
  if (status.machine === undefined) return TEXT.noCartridge;
  const state = status.power === 'off' ? TEXT.powerOff : status.mode === 'running' ? TEXT.running : TEXT.paused;
  return status.cartridge === undefined ? state : `${state} · ${fileName(status.cartridge.path)}`;
}

/**
 * 各按钮是否可用。
 * @param status runtime.status 的结果；还没读到时为 undefined，全部置灰。
 */
export function controlAvailability(status: RuntimeStatusResult | undefined): ControlAvailability {
  const loaded = status?.machine !== undefined;
  const powered = loaded && status?.power === 'on';
  return { powerToggle: loaded, pauseToggle: powered, softReset: powered, hardReset: loaded };
}
