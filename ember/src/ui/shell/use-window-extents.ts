/*
 * ------------------------------------------------------------------------
 * 名称：use-window-extents.ts
 * 说明：窗口随面板伸缩的界面侧处理：把展开的面板占用的尺寸告诉主进程，并在主进程调整窗口期间把页面区钉在原来的像素尺寸上，避免画面闪一下或变形。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：窗口框架在浏览器绘制前通知尺寸，这里同一时刻把页面区钉住；主进程返回后再等两帧（窗口缩放结果传到页面需要时间）才放开；
 *       连续多次通知时只有最后一次的返回才放开；拖动分隔条期间不发送，拖动结束再发最终尺寸；全屏时不发送，全屏里窗口大小由屏幕决定；
 *       页面区没有尺寸监听能力（如测试环境）时不钉住。
 * ------------------------------------------------------------------------
 */

import { useCallback, useRef, useState, type CSSProperties, type RefCallback } from 'react';
import type { PanelExtents } from '../../contracts/shell/window-control';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';

/** 页面区的像素尺寸。 */
interface PagesSize {
  readonly width: number;
  readonly height: number;
}

/** 窗口调整期间页面区要钉住的样式；没有钉住时为空。 */
export type PinnedStyle = Pick<CSSProperties, 'width' | 'height'> | undefined;

/** 窗口随面板伸缩的界面侧处理的结果。 */
export interface WindowExtents {
  /** 挂在页面区容器上的引用，用来量它的稳定尺寸。 */
  readonly pagesRef: RefCallback<HTMLDivElement>;
  /** 页面区容器在窗口调整期间的钉住样式。 */
  readonly pinnedStyle: PinnedStyle;
  /** 传给窗口框架的 onPanelExtentsChange。 */
  readonly onPanelExtentsChange: (extents: PanelExtents, resizing: boolean) => void;
}

/** 等两帧：窗口缩放的结果传到页面之后再放开。 */
function afterTwoFrames(callback: () => void): void {
  requestAnimationFrame(() => requestAnimationFrame(callback));
}

/**
 * 窗口随面板伸缩的界面侧处理。
 * @param bridge 预加载脚本暴露的接口。
 * @param fullScreen 窗口是否全屏。
 */
export function useWindowExtents(bridge: EmberBridge, fullScreen: boolean): WindowExtents {
  const [pinned, setPinned] = useState<PagesSize | null>(null);
  const stableSize = useRef<PagesSize | null>(null);
  const isPinned = useRef(false);
  const lastSent = useRef<PanelExtents | null>(null);
  const sequence = useRef(0);

  const pagesRef = useCallback((element: HTMLDivElement | null) => {
    if (element === null || typeof ResizeObserver === 'undefined') return undefined;
    const measure = (): void => {
      if (!isPinned.current) stableSize.current = { width: element.clientWidth, height: element.clientHeight };
    };
    measure();
    const observer = new ResizeObserver(measure);
    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  const onPanelExtentsChange = useCallback(
    (extents: PanelExtents, resizing: boolean) => {
      if (resizing || fullScreen) return;
      const previous = lastSent.current;
      if (previous !== null && previous.left === extents.left && previous.right === extents.right && previous.bottom === extents.bottom) return;
      lastSent.current = extents;
      sequence.current += 1;
      const current = sequence.current;
      const size = stableSize.current;
      // 第一次是启动时的全部折叠，窗口不会变，不用钉住。
      if (previous !== null && size !== null) {
        isPinned.current = true;
        setPinned(size);
      }
      void bridge.shell.setPanelExtents(extents).then(() =>
        afterTwoFrames(() => {
          if (current !== sequence.current || !isPinned.current) return;
          isPinned.current = false;
          setPinned(null);
        })
      );
    },
    [bridge, fullScreen]
  );

  return { pagesRef, pinnedStyle: pinned === null ? undefined : { width: pinned.width, height: pinned.height }, onPanelExtentsChange };
}
