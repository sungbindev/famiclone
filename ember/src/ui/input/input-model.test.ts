/*
 * ------------------------------------------------------------------------
 * 名称：input-model.test.ts
 * 说明：手柄设置纯逻辑的测试：键位显示文字、键盘键的绑定与解除（物理键不重复）、手柄列表与第几个、指定实体手柄、按钮与轴的绑定、学习时认出被按下的来源、错误说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { PadSlot } from '../../contracts/input/generated/input';
import {
  assignDevice,
  baselineOf,
  bindGamepad,
  bindingOf,
  bindKey,
  clearGamepad,
  clearKey,
  detectSource,
  deviceKey,
  deviceLabel,
  errorText,
  findPad,
  keyLabel,
  keyOf,
  listDevices,
  parseDeviceKey,
  sourceLabel,
  type GamepadLike
} from './input-model';

function pad(index: number, id: string, pressed: boolean[] = [], axes: number[] = [], connected = true): GamepadLike {
  return { index, id, connected, buttons: pressed.map((value) => ({ pressed: value })), axes };
}

const EMPTY: PadSlot[] = [{ keys: [] }, { keys: [] }];

describe('keyLabel', () => {
  it('字母、数字、方向键、小键盘与修饰键显示成易读的文字，其余原样显示', () => {
    expect(keyLabel('KeyZ')).toBe('Z');
    expect(keyLabel('Digit7')).toBe('7');
    expect(keyLabel('ArrowUp')).toBe('↑');
    expect(keyLabel('Enter')).toBe('回车');
    expect(keyLabel('Numpad1')).toBe('小键盘 1');
    expect(keyLabel('ShiftRight')).toBe('右 Shift');
    expect(keyLabel('ControlLeft')).toBe('左 Control');
    expect(keyLabel('F5')).toBe('F5');
  });
});

describe('键盘键的绑定', () => {
  it('绑定后能读出；同一个键再绑定时替换原来的物理键', () => {
    const first = bindKey(EMPTY, 0, 'a', 'KeyX');
    expect(keyOf(first[0], 'a')).toBe('KeyX');
    const second = bindKey(first, 0, 'a', 'KeyJ');
    expect(first[0]?.keys).toEqual([{ button: 'a', code: 'KeyX' }]);
    expect(second[0]?.keys).toEqual([{ button: 'a', code: 'KeyJ' }]);
  });

  it('一个物理键只能绑给一个手柄的键：绑给另一个键或另一个玩家时，原来的绑定被解除', () => {
    const slots = bindKey(bindKey(EMPTY, 0, 'a', 'KeyX'), 1, 'b', 'KeyM');
    const moved = bindKey(slots, 0, 'b', 'KeyX');
    expect(keyOf(moved[0], 'a')).toBeUndefined();
    expect(keyOf(moved[0], 'b')).toBe('KeyX');
    const stolen = bindKey(moved, 1, 'start', 'KeyX');
    expect(keyOf(stolen[0], 'b')).toBeUndefined();
    expect(keyOf(stolen[1], 'start')).toBe('KeyX');
    expect(keyOf(stolen[1], 'b')).toBe('KeyM');
  });

  it('解除绑定', () => {
    const slots = clearKey(bindKey(EMPTY, 0, 'a', 'KeyX'), 0, 'a');
    expect(keyOf(slots[0], 'a')).toBeUndefined();
  });
});

describe('手柄列表', () => {
  it('只列已连接的，按编号排序，同一个设备标识连着多个时算出第几个', () => {
    const devices = listDevices([null, pad(1, 'Twin'), pad(0, 'Solo'), pad(3, 'Twin'), pad(2, 'Gone', [], [], false)]);
    expect(devices).toEqual([
      { id: 'Solo', occurrence: 0 },
      { id: 'Twin', occurrence: 0 },
      { id: 'Twin', occurrence: 1 }
    ]);
  });

  it('选项值与设备互相还原，标识里含冒号也不影响', () => {
    const device = { id: 'Pad: A (Vendor: 1)', occurrence: 2 };
    expect(parseDeviceKey(deviceKey(device))).toEqual(device);
    expect(parseDeviceKey('不是选项值')).toBeUndefined();
  });

  it('显示名：第几个与未连接', () => {
    expect(deviceLabel({ id: 'Pad', occurrence: 0 }, true)).toBe('Pad');
    expect(deviceLabel({ id: 'Pad', occurrence: 1 }, true)).toBe('Pad（第 2 个）');
    expect(deviceLabel({ id: 'Pad', occurrence: 0 }, false)).toBe('Pad（未连接）');
  });

  it('按设备找到对应的手柄：同一个标识里按编号排的第几个', () => {
    const pads = [pad(4, 'Twin'), null, pad(1, 'Twin'), pad(2, 'Twin', [], [], false)];
    expect(findPad(pads, { id: 'Twin', occurrence: 0 })?.index).toBe(1);
    expect(findPad(pads, { id: 'Twin', occurrence: 1 })?.index).toBe(4);
    expect(findPad(pads, { id: 'Twin', occurrence: 2 })).toBeUndefined();
    expect(findPad(pads, { id: 'Other', occurrence: 0 })).toBeUndefined();
  });
});

describe('指定实体手柄与按钮绑定', () => {
  const device = { id: 'Pad', occurrence: 0 };

  it('指定后可以绑定按钮与轴；没有指定实体手柄时绑定不生效', () => {
    expect(bindGamepad(EMPTY, 0, 'a', { kind: 'button', index: 1 })[0]?.gamepad).toBeUndefined();
    const assigned = assignDevice(EMPTY, 0, device);
    const bound = bindGamepad(bindGamepad(assigned, 0, 'a', { kind: 'button', index: 1 }), 0, 'left', { kind: 'axis', index: 0, direction: 'negative' });
    expect(bindingOf(bound[0]?.gamepad, 'a')).toEqual({ button: 'a', kind: 'button', index: 1 });
    expect(bindingOf(bound[0]?.gamepad, 'left')).toEqual({ button: 'left', kind: 'axis', index: 0, direction: 'negative' });
    const rebound = bindGamepad(bound, 0, 'a', { kind: 'button', index: 2 });
    expect(bindingOf(rebound[0]?.gamepad, 'a')?.index).toBe(2);
    expect(rebound[0]?.gamepad?.bindings).toHaveLength(2);
  });

  it('解除按钮绑定', () => {
    const bound = bindGamepad(assignDevice(EMPTY, 0, device), 0, 'a', { kind: 'button', index: 1 });
    expect(bindingOf(clearGamepad(bound, 0, 'a')[0]?.gamepad, 'a')).toBeUndefined();
  });

  it('换成别的手柄时按钮绑定清空，指定的还是同一个手柄时保留；键盘绑定不受影响', () => {
    const bound = bindGamepad(assignDevice(bindKey(EMPTY, 0, 'a', 'KeyX'), 0, device), 0, 'a', { kind: 'button', index: 1 });
    expect(assignDevice(bound, 0, device)[0]?.gamepad?.bindings).toHaveLength(1);
    const other = assignDevice(bound, 0, { id: 'Other', occurrence: 0 });
    expect(other[0]?.gamepad).toEqual({ id: 'Other', occurrence: 0, bindings: [] });
    expect(keyOf(other[0], 'a')).toBe('KeyX');
  });

  it('取消指定：去掉实体手柄，保留键盘绑定', () => {
    const bound = assignDevice(bindKey(EMPTY, 0, 'a', 'KeyX'), 0, device);
    const cleared = assignDevice(bound, 0, undefined);
    expect(cleared[0]).toEqual({ keys: [{ button: 'a', code: 'KeyX' }] });
  });

  it('同一个手柄只能属于一个玩家：指定给另一个玩家时从原来的玩家名下取消', () => {
    const first = assignDevice(EMPTY, 0, device);
    const second = assignDevice(first, 1, device);
    expect(second[0]?.gamepad).toBeUndefined();
    expect(second[1]?.gamepad?.id).toBe('Pad');
  });

  it('来源的显示文字', () => {
    expect(sourceLabel({ kind: 'button', index: 3 })).toBe('按钮 3');
    expect(sourceLabel({ kind: 'axis', index: 1, direction: 'negative' })).toBe('轴 1 负向');
    expect(sourceLabel({ kind: 'axis', index: 1, direction: 'positive' })).toBe('轴 1 正向');
  });
});

describe('学习：认出被按下的来源', () => {
  it('没有变化时认不出；按钮从没按下变成按下时认出按钮', () => {
    const base = baselineOf(pad(0, 'Pad', [false, false], [0, 0]));
    expect(detectSource(base, pad(0, 'Pad', [false, false], [0, 0]))).toBeUndefined();
    expect(detectSource(base, pad(0, 'Pad', [false, true], [0, 0]))).toEqual({ kind: 'button', index: 1 });
  });

  it('开始学习时就按着的按钮不算', () => {
    const base = baselineOf(pad(0, 'Pad', [true, false], []));
    expect(detectSource(base, pad(0, 'Pad', [true, false], []))).toBeUndefined();
  });

  it('轴离开基准并超过 0.5 时认出方向；没过 0.5 不算', () => {
    const base = baselineOf(pad(0, 'Pad', [], [0, 0]));
    expect(detectSource(base, pad(0, 'Pad', [], [0.4, 0]))).toBeUndefined();
    expect(detectSource(base, pad(0, 'Pad', [], [0, -0.9]))).toEqual({ kind: 'axis', index: 1, direction: 'negative' });
    expect(detectSource(base, pad(0, 'Pad', [], [0.8, 0]))).toEqual({ kind: 'axis', index: 0, direction: 'positive' });
  });

  it('静止时在 -1 的扳机轴：推到 1 才认出，停在原位不算', () => {
    const base = baselineOf(pad(0, 'Pad', [], [-1]));
    expect(detectSource(base, pad(0, 'Pad', [], [-1]))).toBeUndefined();
    expect(detectSource(base, pad(0, 'Pad', [], [1]))).toEqual({ kind: 'axis', index: 0, direction: 'positive' });
  });

  it('编号超出核心接受范围的按钮与轴不认', () => {
    const base = baselineOf(pad(0, 'Pad', [], []));
    expect(detectSource(base, pad(0, 'Pad', Array.from({ length: 40 }, (_, index) => index === 35), []))).toBeUndefined();
    expect(detectSource(base, pad(0, 'Pad', [], Array.from({ length: 20 }, (_, index) => (index === 18 ? 1 : 0))))).toBeUndefined();
  });
});

describe('errorText', () => {
  it('取核心给出的说明，没有时给通用说明', () => {
    expect(errorText({ code: 'input.invalidBindings', message: '玩家 1：不合格' })).toBe('玩家 1：不合格');
    expect(errorText(new Error('x'))).toBe('x');
    expect(errorText('oops')).toBe('操作失败。');
  });
});
