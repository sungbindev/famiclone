/*
 * ------------------------------------------------------------------------
 * 名称：use-keyboard-exclusive.ts
 * 说明：键盘独占的界面一侧：画面区获得焦点（且窗口在前台）时请主进程独占键盘，把独占期间的物理按键交给键盘采集；失去焦点、窗口失焦或卸载时关闭独占并松开全部按键。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：独占期间应用里所有窗口都收不到键盘消息，所以只在画面区真正持有焦点时开启；主进程在窗口失焦、隐藏、最小化、关闭与页面重新加载时也会自动关闭，
 *       收到“已关闭”的通知后这里同步状态，窗口回到前台、焦点仍在画面区时再次开启；
 *       不支持独占的平台，主进程的开启请求返回 false，页面内的键盘事件继续工作，这里不需要区分。
 * ------------------------------------------------------------------------
 */

import { useEffect, type RefObject } from 'react';
import type { ShellBridge } from '../../contracts/shell/shell-bridge';
import type { KeyboardCapture } from './key-capture';

/**
 * 画面区的键盘独占。
 * @param shell 外壳的能力。
 * @param screenRef 画面区元素的引用。
 * @param keyboard 键盘采集。
 */
export function useKeyboardExclusive(shell: ShellBridge, screenRef: RefObject<HTMLDivElement | null>, keyboard: KeyboardCapture): void {
  useEffect(() => {
    const element = screenRef.current;
    if (element === null) return undefined;
    /** 已向主进程要求的状态。 */
    let wanted = false;
    /** 现在是否应该独占：窗口在前台且焦点在画面区。 */
    const apply = (): void => {
      const should = document.hasFocus() && document.activeElement === element;
      if (should === wanted) return;
      wanted = should;
      if (!should) keyboard.releaseAll();
      void shell.setKeyboardCapture(should);
    };
    const stopKeys = shell.onNativeKey((event) => keyboard.nativeKey(event.code, event.down));
    const stopReleased = shell.onKeyboardReleased(() => {
      wanted = false;
      keyboard.releaseAll();
    });
    element.addEventListener('focus', apply);
    element.addEventListener('blur', apply);
    window.addEventListener('focus', apply);
    window.addEventListener('blur', apply);
    apply();
    return () => {
      element.removeEventListener('focus', apply);
      element.removeEventListener('blur', apply);
      window.removeEventListener('focus', apply);
      window.removeEventListener('blur', apply);
      stopKeys();
      stopReleased();
      keyboard.releaseAll();
      if (wanted) void shell.setKeyboardCapture(false);
    };
  }, [shell, screenRef, keyboard]);
}
