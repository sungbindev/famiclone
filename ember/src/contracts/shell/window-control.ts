/*
 * ------------------------------------------------------------------------
 * 名称：window-control.ts
 * 说明：窗口控制的契约：界面可以发起的窗口动作白名单、窗口状态，以及主进程实现窗口动作的接口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：窗口按钮由系统绘制（见 platform/window/app-window.ts），所以最小化与最大化只给快捷入口和以后的菜单使用；
 *       界面发来的动作名先经 isWindowAction 检查，不在白名单里的一律丢弃；
 *       面板展开时窗口向外伸展（页面区大小不变），界面把展开的面板占用的尺寸交给 setPanelExtents，窗口控制器据此伸缩窗口并更新最小尺寸。
 * ------------------------------------------------------------------------
 */

import type { PanelExtents } from '../../shared/shell-state/panel-extents';

export type { PanelExtents } from '../../shared/shell-state/panel-extents';

/** 界面可以发起的窗口动作。 */
export const WINDOW_ACTIONS = ['minimize', 'toggleMaximize', 'toggleFullScreen', 'close', 'quit'] as const;

/** 窗口动作：最小化、最大化与还原、全屏与退出全屏、关闭窗口（全部窗口关闭后应用退出）、退出应用。 */
export type WindowAction = (typeof WINDOW_ACTIONS)[number];

/** 判断收到的值是不是白名单里的窗口动作。 */
export function isWindowAction(value: unknown): value is WindowAction {
  return typeof value === 'string' && (WINDOW_ACTIONS as readonly string[]).includes(value);
}

/** 窗口当前的状态。 */
export interface WindowState {
  readonly maximized: boolean;
  readonly fullScreen: boolean;
}

/** 窗口动作的执行者，由主进程的窗口实现。 */
export interface WindowController {
  /** 执行动作；没有窗口时什么都不做。 */
  perform(action: WindowAction): void;
  /** 窗口当前的状态；没有窗口时都是 false。 */
  state(): WindowState;
  /** 展开的面板占用的尺寸变了：窗口按差值向外伸缩（最大化与全屏时只记录，不改窗口大小），并更新最小尺寸。 */
  setPanelExtents(extents: PanelExtents): void;
  /** 当前记录的面板占用尺寸；窗口换了一个时重新从全部折叠算起。 */
  panelExtents(): PanelExtents;
}
