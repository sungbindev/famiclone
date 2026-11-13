/*
 * ------------------------------------------------------------------------
 * 名称：runtime-status.test.ts
 * 说明：运行页状态文字与按钮可用性规则的测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';
import { controlAvailability, statusText } from './runtime-status';

/** 构造状态：machine 为 undefined 表示没有卡带。 */
function status(power: 'off' | 'on', mode: 'paused' | 'running', machine?: string): RuntimeStatusResult {
  const base: RuntimeStatusResult = { power, mode, frame: 0, cycles: 0, speed: 100 };
  if (machine === undefined) return base;
  return { ...base, machine, cartridge: { path: 'D:\\roms\\nestest.nes', host: machine } as RuntimeStatusResult['cartridge'] };
}

describe('statusText', () => {
  it('还没读到状态时为空', () => {
    expect(statusText(undefined)).toBe('');
  });

  it('没有卡带时显示未装入卡带', () => {
    expect(statusText(status('off', 'paused'))).toBe('未装入卡带');
  });

  it('按电源与运行模式显示，后面带卡带文件名', () => {
    expect(statusText(status('off', 'paused', 'fc'))).toBe('已断电 · nestest.nes');
    expect(statusText(status('on', 'paused', 'fc'))).toBe('已暂停 · nestest.nes');
    expect(statusText(status('on', 'running', 'fc'))).toBe('运行中 · nestest.nes');
  });

  it('文件名取路径最后一段，两种分隔符都认', () => {
    const value = status('on', 'running', 'fc');
    if (value.cartridge !== undefined) value.cartridge.path = '/home/me/roms/mario.nes';
    expect(statusText(value)).toBe('运行中 · mario.nes');
  });
});

describe('controlAvailability', () => {
  it('还没读到状态或没有卡带时全部置灰', () => {
    const none = { powerToggle: false, pauseToggle: false, softReset: false, hardReset: false };
    expect(controlAvailability(undefined)).toEqual(none);
    expect(controlAvailability(status('off', 'paused'))).toEqual(none);
  });

  it('断电时只有上电与硬复位可用', () => {
    expect(controlAvailability(status('off', 'paused', 'fc'))).toEqual({ powerToggle: true, pauseToggle: false, softReset: false, hardReset: true });
  });

  it('通电时全部可用', () => {
    expect(controlAvailability(status('on', 'running', 'fc'))).toEqual({ powerToggle: true, pauseToggle: true, softReset: true, hardReset: true });
    expect(controlAvailability(status('on', 'paused', 'fc'))).toEqual({ powerToggle: true, pauseToggle: true, softReset: true, hardReset: true });
  });
});
