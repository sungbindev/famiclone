/*
 * ------------------------------------------------------------------------
 * 名称：use-gamepad-devices.ts
 * 说明：系统发现的手柄：定时读取浏览器的手柄列表并在手柄接入、拔出时刷新，只在列表变化时更新界面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：页面显示在独立的对话框窗口里，手柄要从页面所在窗口的 navigator 读取（不是主窗口的），所以经页面里的元素找到它所在的窗口；
 *       浏览器要等手柄上有一次按键后才会把它交给页面。
 * ------------------------------------------------------------------------
 */

import { useEffect, useState, type RefObject } from 'react';
import { listDevices, type GamepadLike, type PadDevice } from './input-model';

/** 刷新手柄列表的间隔（毫秒）。 */
const REFRESH_INTERVAL = 500;

/** 元素所在的窗口（对话框是独立的窗口）；取不到时用当前窗口。 */
export function ownerView(element: Element | null): Window {
  return element?.ownerDocument.defaultView ?? window;
}

/** 读取窗口里的手柄列表；浏览器不支持时为空。 */
export function readPads(view: Window): ArrayLike<GamepadLike | null> {
  return typeof view.navigator.getGamepads === 'function' ? view.navigator.getGamepads() : [];
}

/** 两个设备列表是否一样。 */
function sameDevices(left: readonly PadDevice[], right: readonly PadDevice[]): boolean {
  return left.length === right.length && left.every((device, index) => device.id === right[index]?.id && device.occurrence === right[index]?.occurrence);
}

/**
 * 系统发现的手柄列表，随接入与拔出自动更新。
 * @param rootRef 页面里的一个元素，用来找到页面所在的窗口。
 */
export function useGamepadDevices(rootRef: RefObject<HTMLElement | null>): readonly PadDevice[] {
  const [devices, setDevices] = useState<readonly PadDevice[]>([]);

  useEffect(() => {
    const view = ownerView(rootRef.current);
    const refresh = (): void => {
      const next = listDevices(readPads(view));
      setDevices((current) => (sameDevices(current, next) ? current : next));
    };
    refresh();
    const timer = view.setInterval(refresh, REFRESH_INTERVAL);
    view.addEventListener('gamepadconnected', refresh);
    view.addEventListener('gamepaddisconnected', refresh);
    return () => {
      view.clearInterval(timer);
      view.removeEventListener('gamepadconnected', refresh);
      view.removeEventListener('gamepaddisconnected', refresh);
    };
  }, [rootRef]);

  return devices;
}
