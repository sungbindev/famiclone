/*
 * ------------------------------------------------------------------------
 * 名称：runtime-status.test.ts
 * 说明：运行页状态规则的测试：机器状态（未开机、已断电、已暂停、运行中、还没读到）、各控制是否可用、边栏与状态栏的显示文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { machineStatus } from './runtime-fakes';
import { cartridgeText, controlAvailability, fileNameOf, hostText, machineState, pauseToggleText, pluginText, powerToggleText, stateText, volumeText } from './runtime-status';

const NONE = machineStatus('off', 'paused');
const OFF = machineStatus('off', 'paused', 'D:\\roms\\nestest.nes');
const PAUSED = machineStatus('on', 'paused', 'D:\\roms\\nestest.nes');
const RUNNING = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');

describe('machineState 与 stateText', () => {
  it('还没读到状态时是 unknown，显示“—”', () => {
    expect(machineState(undefined)).toBe('unknown');
    expect(stateText(undefined)).toBe('—');
  });

  it('没有卡带（没有开机）是未开机，不管电源字段是什么', () => {
    expect(machineState(NONE)).toBe('none');
    expect(stateText(NONE)).toBe('未开机');
    expect(stateText({ ...NONE, power: 'on', mode: 'running' })).toBe('未开机');
  });

  it('开机后按电源与运行模式：已断电、已暂停、运行中', () => {
    expect(stateText(OFF)).toBe('已断电');
    expect(stateText(PAUSED)).toBe('已暂停');
    expect(stateText(RUNNING)).toBe('运行中');
  });
});

describe('controlAvailability', () => {
  it('还没读到状态或没有开机时全部置灰', () => {
    const none = { powerToggle: false, pauseToggle: false, softReset: false, hardReset: false, eject: false };
    expect(controlAvailability(undefined)).toEqual(none);
    expect(controlAvailability(NONE)).toEqual(none);
  });

  it('断电时：上电、硬复位、关闭卡带可用，暂停与软复位置灰', () => {
    expect(controlAvailability(OFF)).toEqual({ powerToggle: true, pauseToggle: false, softReset: false, hardReset: true, eject: true });
  });

  it('暂停与运行中时全部可用', () => {
    const all = { powerToggle: true, pauseToggle: true, softReset: true, hardReset: true, eject: true };
    expect(controlAvailability(PAUSED)).toEqual(all);
    expect(controlAvailability(RUNNING)).toEqual(all);
  });

  it('关闭卡带只在装着卡带时可用', () => {
    expect(controlAvailability({ ...RUNNING, cartridge: undefined }).eject).toBe(false);
  });
});

describe('显示文字', () => {
  it('主机型号：经典 FC；没有开机时是“—”', () => {
    expect(hostText(RUNNING)).toBe('经典 FC');
    expect(hostText(NONE)).toBe('—');
    expect(hostText(undefined)).toBe('—');
    expect(hostText({ ...RUNNING, machine: 'sb2000' })).toBe('sb2000');
  });

  it('卡带：文件名，没有卡带时是“无”；两种路径分隔符都认', () => {
    expect(cartridgeText(RUNNING)).toBe('nestest.nes');
    expect(cartridgeText(NONE)).toBe('无');
    expect(cartridgeText(undefined)).toBe('无');
    expect(fileNameOf('/home/me/mario.nes')).toBe('mario.nes');
  });

  it('mapper 插件：插件名，没有卡带时是“—”', () => {
    expect(pluginText(RUNNING)).toBe('NROM');
    expect(pluginText(NONE)).toBe('—');
  });

  it('上电或断电、暂停或继续的名称随状态变化', () => {
    expect(powerToggleText(OFF)).toBe('上电');
    expect(powerToggleText(RUNNING)).toBe('断电');
    expect(powerToggleText(undefined)).toBe('上电');
    expect(pauseToggleText(RUNNING)).toBe('暂停');
    expect(pauseToggleText(PAUSED)).toBe('继续');
    expect(pauseToggleText(OFF)).toBe('继续');
  });

  it('音量文字：静音时显示“已静音”', () => {
    expect(volumeText(80, false)).toBe('音量 80%');
    expect(volumeText(80, true)).toBe('已静音');
  });
});
