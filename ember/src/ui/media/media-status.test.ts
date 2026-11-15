/*
 * ------------------------------------------------------------------------
 * 名称：media-status.test.ts
 * 说明：设备面板纯规则的测试：盘片的显示文字、软驱的状态文字、打印机的状态文字、磁带机的磁带与状态与位置文字、各按钮是否可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { floppyStatus, tapeStatus } from './media-fakes';
import { diskText, floppyAvailability, floppyStateText, printerStateText, tapeAvailability, tapeNameText, tapePositionText, tapeStateText } from './media-status';

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

  it('软驱没有连接（主机没有这个软驱）时全部不可用，文字按空盘', () => {
    expect(floppyAvailability(undefined)).toEqual({ insert: false, eject: false, save: false, writeProtect: false });
    expect([diskText(undefined), floppyStateText(undefined)]).toEqual(['空', '无盘片']);
  });
});

describe('磁带机的显示文字', () => {
  it('磁带没有时显示“空”“无磁带”“—”，有磁带时显示文件名、状态与“当前 / 总时长”', () => {
    const empty = tapeStatus({ connected: true });
    expect([tapeNameText(empty), tapeStateText(empty), tapePositionText(empty)]).toEqual(['空', '无磁带', '—']);
    const stopped = tapeStatus({ connected: true, loaded: true, fileName: 'lesson.m4a', position: 75.9, duration: 1689.57 });
    expect([tapeNameText(stopped), tapeStateText(stopped), tapePositionText(stopped)]).toEqual(['lesson.m4a', '已停止', '1:15 / 28:09']);
    expect(tapeStateText({ ...stopped, mode: 'playing' })).toBe('播放中');
    expect(tapeStateText({ ...stopped, mode: 'recording' })).toBe('录音中');
  });

  it('超过一小时写成“时:分:秒”，总时长不知道时只写当前位置', () => {
    expect(tapePositionText(tapeStatus({ loaded: true, position: 3725, duration: 7200 }))).toBe('1:02:05 / 2:00:00');
    expect(tapePositionText(tapeStatus({ loaded: true, position: 5, duration: 0 }))).toBe('0:05');
  });
});

describe('磁带机按钮的可用性', () => {
  it('主机已通电、停止时：没有磁带只能播放（选磁带）与录音，有磁带取出、播放、录音可用但不能停止', () => {
    expect(tapeAvailability(tapeStatus({ connected: true }))).toEqual({ eject: false, play: true, record: true, stop: false });
    expect(tapeAvailability(tapeStatus({ connected: true, loaded: true }))).toEqual({ eject: true, play: true, record: true, stop: false });
  });

  it('主机没有通电：没有磁带时播放可点（载入磁带），有磁带时只能取出，播放与录音置灰', () => {
    expect(tapeAvailability(tapeStatus({ connected: false }))).toEqual({ eject: false, play: true, record: false, stop: false });
    expect(tapeAvailability(tapeStatus({ connected: false, loaded: true }))).toEqual({ eject: true, play: false, record: false, stop: false });
  });

  it('播放或录音时只有停止与取出可用', () => {
    for (const mode of ['playing', 'recording'] as const) {
      expect(tapeAvailability(tapeStatus({ connected: true, loaded: true, mode }))).toEqual({ eject: true, play: false, record: false, stop: true });
    }
  });

  it('状态还没读到时全部不可用，文字按无磁带', () => {
    expect(tapeAvailability(undefined)).toEqual({ eject: false, play: false, record: false, stop: false });
    expect([tapeNameText(undefined), tapeStateText(undefined), tapePositionText(undefined)]).toEqual(['空', '无磁带', '—']);
  });
});
