/*
 * ------------------------------------------------------------------------
 * 名称：runtime-status.test.ts
 * 说明：运行模块状态规则的测试：机器状态（未开机、已断电、已暂停、运行中、还没读到）、开机按钮这次要做什么与各控制是否可用（按已选机型）、鼠标捕获，以及显示文字（没有信息时为 undefined，不显示）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { machineStatus } from './runtime-fakes';
import { capturesMouse, cartridgeText, controlAvailability, fileNameOf, hostText, machineState, pauseToggleText, pluginText, powerToggleAction, powerToggleText, stateText } from './runtime-status';

const NONE = machineStatus('off', 'paused');
const OFF = machineStatus('off', 'paused', 'D:\\roms\\nestest.nes');
const PAUSED = machineStatus('on', 'paused', 'D:\\roms\\nestest.nes');
const RUNNING = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');
const WHOLE_OFF = { ...NONE, machine: 'sb2000' };

/** 机型清单：小霸王 2000 已支持，是否有 BIOS 由参数决定。 */
function machines(withBios: boolean) {
  return [
    { id: 'fc', supported: true },
    { id: 'sb97', supported: false },
    { id: 'sb2000', supported: true, ...(withBios ? { bios: { fileName: 'SB2K_BIOS.NES' } } : {}) }
  ];
}

describe('machineState 与 stateText', () => {
  it('还没读到状态时是 unknown，没有文字', () => {
    expect(machineState(undefined)).toBe('unknown');
    expect(stateText(undefined)).toBeUndefined();
  });

  it('没有开机是未开机，不管电源字段是什么', () => {
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

describe('powerToggleAction', () => {
  it('通电时是断电，不管已选什么机型', () => {
    expect(powerToggleAction(RUNNING, 'fc', machines(false))).toBe('powerOff');
    expect(powerToggleAction(RUNNING, 'sb2000', machines(true))).toBe('powerOff');
  });

  it('已选经典 FC：装着 FC 卡带的断电机器直接上电；没有装 FC 卡带时不能做', () => {
    expect(powerToggleAction(OFF, 'fc', machines(false))).toBe('powerOn');
    expect(powerToggleAction(NONE, 'fc', machines(false))).toBe('none');
    expect(powerToggleAction(WHOLE_OFF, 'fc', machines(true))).toBe('none');
  });

  it('已选学习机：已支持并已添加 BIOS 就开机，不需要卡带，当前装的是别的主机也重新开机；没有 BIOS 或不支持时不能做', () => {
    expect(powerToggleAction(NONE, 'sb2000', machines(true))).toBe('boot');
    expect(powerToggleAction(OFF, 'sb2000', machines(true))).toBe('boot');
    expect(powerToggleAction(NONE, 'sb2000', machines(false))).toBe('none');
    expect(powerToggleAction(NONE, 'sb97', machines(true))).toBe('none');
    expect(powerToggleAction(NONE, 'nope', machines(true))).toBe('none');
  });

  it('还没读到状态时不能做', () => {
    expect(powerToggleAction(undefined, 'fc', machines(true))).toBe('none');
  });
});

describe('controlAvailability', () => {
  it('还没读到状态或没有开机、已选经典 FC 时全部置灰', () => {
    const none = { powerToggle: false, pauseToggle: false, softReset: false, hardReset: false, eject: false };
    expect(controlAvailability(undefined, 'fc', machines(false))).toEqual(none);
    expect(controlAvailability(NONE, 'fc', machines(false))).toEqual(none);
  });

  it('没有开机、已选学习机且有 BIOS：只有开机可用', () => {
    expect(controlAvailability(NONE, 'sb2000', machines(true))).toEqual({ powerToggle: true, pauseToggle: false, softReset: false, hardReset: false, eject: false });
  });

  it('断电时：开机、硬复位、关闭卡带可用，暂停与软复位置灰', () => {
    expect(controlAvailability(OFF, 'fc', machines(false))).toEqual({ powerToggle: true, pauseToggle: false, softReset: false, hardReset: true, eject: true });
  });

  it('暂停与运行中时全部可用', () => {
    const all = { powerToggle: true, pauseToggle: true, softReset: true, hardReset: true, eject: true };
    expect(controlAvailability(PAUSED, 'fc', machines(false))).toEqual(all);
    expect(controlAvailability(RUNNING, 'fc', machines(false))).toEqual(all);
  });

  it('关闭卡带只在装着卡带时可用', () => {
    expect(controlAvailability({ ...RUNNING, cartridge: undefined }, 'fc', machines(false)).eject).toBe(false);
  });
});

describe('capturesMouse', () => {
  it('设置打开且机器带鼠标（整机机型）才捕获；经典 FC、没有开机、还没读到状态、设置关闭都不捕获', () => {
    expect(capturesMouse(true, { ...RUNNING, machine: 'sb2000' })).toBe(true);
    expect(capturesMouse(true, RUNNING)).toBe(false);
    expect(capturesMouse(true, NONE)).toBe(false);
    expect(capturesMouse(true, undefined)).toBe(false);
    expect(capturesMouse(false, { ...RUNNING, machine: 'sb2000' })).toBe(false);
  });
});

describe('显示文字', () => {
  it('主机型号：经典 FC 与整机机型的显示名；没有开机时没有文字；不认识的型号显示型号标识', () => {
    expect(hostText(RUNNING)).toBe('经典 FC');
    expect(hostText(NONE)).toBeUndefined();
    expect(hostText(undefined)).toBeUndefined();
    expect(hostText({ ...RUNNING, machine: 'sb2000' })).toBe('小霸王 2000');
    expect(hostText({ ...RUNNING, machine: 'other' })).toBe('other');
  });

  it('卡带：文件名，没有卡带（含整机机型用 BIOS 开机）时没有文字；两种路径分隔符都认', () => {
    expect(cartridgeText(RUNNING)).toBe('nestest.nes');
    expect(cartridgeText(NONE)).toBeUndefined();
    expect(cartridgeText(undefined)).toBeUndefined();
    expect(cartridgeText(WHOLE_OFF)).toBeUndefined();
    expect(fileNameOf('/home/me/mario.nes')).toBe('mario.nes');
  });

  it('mapper 插件：插件名，没有卡带时没有文字', () => {
    expect(pluginText(RUNNING)).toBe('NROM');
    expect(pluginText(NONE)).toBeUndefined();
  });

  it('开机或断电、暂停或继续的名称随状态变化', () => {
    expect(powerToggleText(OFF)).toBe('开机');
    expect(powerToggleText(RUNNING)).toBe('断电');
    expect(powerToggleText(undefined)).toBe('开机');
    expect(pauseToggleText(RUNNING)).toBe('暂停');
    expect(pauseToggleText(PAUSED)).toBe('继续');
    expect(pauseToggleText(OFF)).toBe('继续');
  });
});
