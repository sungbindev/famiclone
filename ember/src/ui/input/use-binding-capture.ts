/*
 * ------------------------------------------------------------------------
 * 名称：use-binding-capture.ts
 * 说明：学习绑定：等用户按下键盘上的一个键，或等指定的实体手柄上有一个按钮被按下或一个轴被推到底，认出来后交给调用方保存；同一时间只学习一个键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：页面在独立的对话框窗口里，键盘事件与动画帧都挂在页面所在的窗口上；学习键盘时按 Esc 取消，学习手柄时以开始那一刻手柄的状态为基准，
 *       开始前就按着的按钮不算。
 * ------------------------------------------------------------------------
 */

import { useCallback, useEffect, useRef, useState, type RefObject } from 'react';
import { baselineOf, detectSource, findPad, type PadBaseline, type PadButtonName, type PadDevice, type PadSource } from './input-model';
import { ownerView, readPads } from './use-gamepad-devices';

/** 正在学习的键：键盘的，或指定设备上的手柄的。 */
export type Waiting =
  | { readonly kind: 'key'; readonly slot: number; readonly button: PadButtonName }
  | { readonly kind: 'pad'; readonly slot: number; readonly button: PadButtonName; readonly device: PadDevice };

/** 学习到结果时的回调。 */
export interface CaptureHandlers {
  onKey(slot: number, button: PadButtonName, code: string): void;
  onPad(slot: number, button: PadButtonName, source: PadSource): void;
}

/** 学习的状态与操作。 */
export interface BindingCapture {
  readonly waiting: Waiting | null;
  startKey(slot: number, button: PadButtonName): void;
  startPad(slot: number, button: PadButtonName, device: PadDevice): void;
  cancel(): void;
}

/** 取消学习的键。 */
const CANCEL_CODE = 'Escape';

/** 核心接受的物理键位：只含字母与数字、不超过 32 个字符。 */
const KEY_CODE_PATTERN = /^[A-Za-z0-9]{1,32}$/;

/**
 * 学习绑定。
 * @param rootRef 页面里的一个元素，用来找到页面所在的窗口。
 * @param handlers 学到结果时的回调。
 */
export function useBindingCapture(rootRef: RefObject<HTMLElement | null>, handlers: CaptureHandlers): BindingCapture {
  const [waiting, setWaiting] = useState<Waiting | null>(null);
  const handlersRef = useRef(handlers);

  useEffect(() => {
    handlersRef.current = handlers;
  }, [handlers]);

  useEffect(() => {
    if (waiting?.kind !== 'key') return undefined;
    const { slot, button } = waiting;
    const view = ownerView(rootRef.current);
    const onKeyDown = (event: KeyboardEvent): void => {
      // 学习期间的按键不再触发页面里的其他操作。
      event.preventDefault();
      event.stopPropagation();
      if (event.repeat) return;
      if (event.code === CANCEL_CODE) {
        setWaiting(null);
        return;
      }
      if (!KEY_CODE_PATTERN.test(event.code)) return;
      setWaiting(null);
      handlersRef.current.onKey(slot, button, event.code);
    };
    view.document.addEventListener('keydown', onKeyDown, true);
    return () => view.document.removeEventListener('keydown', onKeyDown, true);
  }, [waiting, rootRef]);

  useEffect(() => {
    if (waiting?.kind !== 'pad') return undefined;
    const { slot, button, device } = waiting;
    const view = ownerView(rootRef.current);
    let baseline: PadBaseline | undefined;
    let frame = 0;
    const poll = (): void => {
      const pad = findPad(readPads(view), device);
      if (pad !== undefined) {
        if (baseline === undefined) {
          baseline = baselineOf(pad);
        } else {
          const source = detectSource(baseline, pad);
          if (source !== undefined) {
            setWaiting(null);
            handlersRef.current.onPad(slot, button, source);
            return;
          }
        }
      }
      frame = view.requestAnimationFrame(poll);
    };
    frame = view.requestAnimationFrame(poll);
    return () => view.cancelAnimationFrame(frame);
  }, [waiting, rootRef]);

  const startKey = useCallback((slot: number, button: PadButtonName) => setWaiting({ kind: 'key', slot, button }), []);
  const startPad = useCallback((slot: number, button: PadButtonName, device: PadDevice) => setWaiting({ kind: 'pad', slot, button, device }), []);
  const cancel = useCallback(() => setWaiting(null), []);

  return { waiting, startKey, startPad, cancel };
}
