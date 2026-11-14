/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-capture.test.ts
 * 说明：游戏手柄采集的测试：变化才上报、轴取两位小数、按键与轴的数量上限、拔掉的手柄上报断开、全部断开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { GamepadInputMessage } from '../../contracts/stream-ports/stream-messages';
import { GamepadCapture, type GamepadLike } from './gamepad-capture';

function pad(index: number, pressed: boolean[], axes: number[], connected = true): GamepadLike {
  return { index, connected, buttons: pressed.map((value) => ({ pressed: value })), axes };
}

function setup() {
  const sent: GamepadInputMessage[] = [];
  return { sent, capture: new GamepadCapture((message) => sent.push(message)) };
}

describe('GamepadCapture', () => {
  it('没有手柄时什么也不上报', () => {
    const { sent, capture } = setup();
    capture.poll([null, null, null, null]);
    expect(sent).toEqual([]);
  });

  it('第一次读到就上报整个手柄的状态，状态没变不重复上报', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, [true, false], [0.5, -1])]);
    capture.poll([pad(0, [true, false], [0.5, -1])]);
    expect(sent).toEqual([{ type: 'gamepad', index: 0, connected: true, buttons: [true, false], axes: [0.5, -1] }]);
  });

  it('按键或轴变了才再次上报', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, [false], [0])]);
    capture.poll([pad(0, [true], [0])]);
    capture.poll([pad(0, [true], [0.25])]);
    expect(sent).toHaveLength(3);
  });

  it('轴的值取两位小数并限制在 -1 到 1，微小的抖动不产生消息', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, [], [0.123456, 1.5, -3])]);
    capture.poll([pad(0, [], [0.1234, 1.5, -3])]);
    expect(sent).toEqual([{ type: 'gamepad', index: 0, connected: true, buttons: [], axes: [0.12, 1, -1] }]);
  });

  it('按键与轴超出上限的部分不上报', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, Array.from({ length: 40 }, () => true), Array.from({ length: 20 }, () => 0))]);
    expect(sent[0]?.buttons).toHaveLength(32);
    expect(sent[0]?.axes).toHaveLength(16);
  });

  it('多个手柄各自上报，位置为 null 的跳过', () => {
    const { sent, capture } = setup();
    capture.poll([null, pad(1, [true], []), pad(2, [false], [])]);
    expect(sent.map((message) => message.index)).toEqual([1, 2]);
  });

  it('手柄被拔掉时上报一次断开，之后不再上报', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, [true], [0])]);
    capture.poll([null]);
    capture.poll([null]);
    expect(sent).toEqual([
      { type: 'gamepad', index: 0, connected: true, buttons: [true], axes: [0] },
      { type: 'gamepad', index: 0, connected: false, buttons: [], axes: [] }
    ]);
  });

  it('浏览器给出 connected 为 false 的手柄时只上报一次断开', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, [true], [0])]);
    capture.poll([pad(0, [], [], false)]);
    capture.poll([pad(0, [], [], false)]);
    expect(sent).toHaveLength(2);
    expect(sent[1]).toEqual({ type: 'gamepad', index: 0, connected: false, buttons: [], axes: [] });
  });

  it('全部断开：已知的手柄各上报一次断开', () => {
    const { sent, capture } = setup();
    capture.poll([pad(0, [true], []), pad(1, [false], [])]);
    sent.length = 0;
    capture.disconnectAll();
    expect(sent.map((message) => [message.index, message.connected])).toEqual([
      [0, false],
      [1, false]
    ]);
    sent.length = 0;
    capture.disconnectAll();
    expect(sent).toEqual([]);
  });
});
