/*
 * ------------------------------------------------------------------------
 * 名称：media-status.test.ts
 * 说明：设备面板纯规则的测试：盘片的显示文字、软驱的状态文字、打印机的状态文字、各按钮是否可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { floppyStatus } from './media-fakes';
import { diskText, floppyAvailability, floppyStateText, printerStateText } from './media-status';

describe('盘片的显示文字', () => {
  it('没有盘片显示“空”，有盘片显示镜像名，有未保存的修改时注明', () => {
    expect(diskText(floppyStatus(0))).toBe('空');
    expect(diskText(floppyStatus(0, 'sys.img'))).toBe('sys.img');
    expect(diskText(floppyStatus(0, 'sys.img', { modified: true }))).toBe('sys.img（有未保存的修改）');
  });
});

describe('软驱的状态文字', () => {
  it('无盘片、空闲、读写中带磁道', () => {
    expect(floppyStateText(floppyStatus(0))).toBe('无盘片');
    expect(floppyStateText(floppyStatus(0, 'sys.img'))).toBe('空闲');
    expect(floppyStateText(floppyStatus(0, 'sys.img', { motor: true, cylinder: 12 }))).toBe('读写中（磁道 12）');
  });
});

describe('打印机的状态文字', () => {
  it('空闲、打印中、打印完成；还没读到时按空闲', () => {
    expect(printerStateText(undefined)).toBe('空闲');
    expect(printerStateText({ state: 'idle', hasContent: false, pages: 0 })).toBe('空闲');
    expect(printerStateText({ state: 'printing', hasContent: true, pages: 0 })).toBe('打印中');
    expect(printerStateText({ state: 'done', hasContent: false, pages: 1 })).toBe('打印完成');
  });
});

describe('软驱按钮的可用性', () => {
  it('没有盘片时只能载入镜像；有盘片时全部可用', () => {
    expect(floppyAvailability(floppyStatus(0))).toEqual({ insert: true, eject: false, save: false, writeProtect: false });
    expect(floppyAvailability(floppyStatus(0, 'sys.img'))).toEqual({ insert: true, eject: true, save: true, writeProtect: true });
  });
});
