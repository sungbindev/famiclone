/*
 * ------------------------------------------------------------------------
 * 名称：window-events.ts
 * 说明：窗口事件的监听：窗口移动、改变大小、最大化、还原时报告窗口的位置（非最大化时的左上角坐标），窗口关闭前再报告一次；
 *       最大化、还原、进入与退出全屏、置顶与取消置顶时通知窗口状态变了。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：全屏时窗口边界是整个屏幕，不报告位置，退出全屏后再报告；窗口大小与是否最大化不保存，所以不报告；写盘的防抖由接收方负责。
 * ------------------------------------------------------------------------
 */

/** 被监听的窗口，BrowserWindow 满足。 */
export interface WatchedWindow {
  on(event: string, listener: () => void): unknown;
  isDestroyed(): boolean;
  isFullScreen(): boolean;
  getNormalBounds(): { x: number; y: number; width: number; height: number };
}

/** 窗口的位置，与 shared/shell-state 的 WindowPlacement 结构一致。 */
export interface ReportedPlacement {
  readonly x: number;
  readonly y: number;
}

/** 窗口事件的接收方。 */
export interface WindowEventHandlers {
  /** 位置变了。 */
  readonly onPlacement: (placement: ReportedPlacement) => void;
  /** 最大化、全屏或置顶的状态变了。 */
  readonly onState: () => void;
}

/** 会改变位置的窗口事件。 */
const PLACEMENT_EVENTS = ['move', 'resize', 'maximize', 'unmaximize', 'leave-full-screen', 'close'] as const;
/** 会改变最大化、全屏或置顶状态的窗口事件。 */
const STATE_EVENTS = ['maximize', 'unmaximize', 'enter-full-screen', 'leave-full-screen', 'always-on-top-changed'] as const;

/**
 * 开始监听窗口事件。
 * @param window 主窗口。
 * @param handlers 事件的接收方。
 */
export function watchWindow(window: WatchedWindow, handlers: WindowEventHandlers): void {
  const reportPlacement = (): void => {
    if (window.isDestroyed() || window.isFullScreen()) return;
    const { x, y } = window.getNormalBounds();
    handlers.onPlacement({ x, y });
  };
  for (const event of PLACEMENT_EVENTS) window.on(event, reportPlacement);
  for (const event of STATE_EVENTS) window.on(event, handlers.onState);
}
