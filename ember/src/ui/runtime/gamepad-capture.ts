/*
 * ------------------------------------------------------------------------
 * 名称：gamepad-capture.ts
 * 说明：游戏手柄采集：对比每个手柄上一次的状态，变化时把整个手柄的当前状态转成手柄事件消息；拔掉的手柄上报一次“已断开”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯逻辑，不读取浏览器接口，由画面组件定时把 navigator.getGamepads() 的结果交进来。按键与轴的顺序同浏览器的标准映射；
 *       轴的值取两位小数，微小的抖动不产生消息。界面不解释这些事件，由卡带的 mapper 脚本决定怎么用。
 * ------------------------------------------------------------------------
 */

import type { GamepadInputMessage } from '../../contracts/stream-ports/stream-messages';

/** 上报的按键数与轴数的上限，与核心宿主的校验一致；超出的部分不上报。 */
const MAX_BUTTONS = 32;
const MAX_AXES = 16;

/** 轴的值保留的小数位。 */
const AXIS_SCALE = 100;

/** 采集所需的手柄信息（浏览器的 Gamepad 满足）。 */
export interface GamepadLike {
  readonly index: number;
  readonly connected: boolean;
  readonly buttons: ReadonlyArray<{ readonly pressed: boolean }>;
  readonly axes: readonly number[];
}

/** 手柄采集。 */
export class GamepadCapture {
  /** 每个手柄上一次上报的内容（序列化成文字，便于比较）。 */
  private readonly reported = new Map<number, string>();

  /** @param send 手柄状态变化时调用。 */
  constructor(private readonly send: (message: GamepadInputMessage) => void) {}

  /**
   * 处理一轮读到的手柄：状态变了的上报，原来有现在没有的上报断开。
   * @param pads navigator.getGamepads() 的结果，没有手柄的位置为 null。
   */
  poll(pads: ArrayLike<GamepadLike | null>): void {
    const present = new Set<number>();
    for (let position = 0; position < pads.length; position += 1) {
      const pad = pads[position];
      if (pad === null || pad === undefined) continue;
      present.add(pad.index);
      this.report({
        type: 'gamepad',
        index: pad.index,
        connected: pad.connected,
        buttons: pad.buttons.slice(0, MAX_BUTTONS).map((button) => button.pressed),
        axes: pad.axes.slice(0, MAX_AXES).map((value) => Math.round(Math.max(-1, Math.min(1, value)) * AXIS_SCALE) / AXIS_SCALE)
      });
    }
    for (const index of this.reported.keys()) {
      if (!present.has(index)) this.disconnect(index);
    }
  }

  /** 全部手柄断开：页面卸载时调用，核心里的脚本据此松开手柄上的按键。 */
  disconnectAll(): void {
    for (const index of this.reported.keys()) this.disconnect(index);
  }

  private disconnect(index: number): void {
    this.report({ type: 'gamepad', index, connected: false, buttons: [], axes: [] });
    this.reported.delete(index);
  }

  /** 与上一次不同才发送。 */
  private report(message: GamepadInputMessage): void {
    const text = JSON.stringify(message);
    if (this.reported.get(message.index) === text) return;
    this.reported.set(message.index, text);
    this.send(message);
  }
}
