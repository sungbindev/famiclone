/*
 * ------------------------------------------------------------------------
 * 名称：window-controller.ts
 * 说明：窗口动作的执行：最小化、最大化与还原、全屏与退出全屏、关闭窗口、退出应用，并读出窗口的最大化与全屏状态；
 *       面板展开或折叠时按面板占用的尺寸向外伸缩窗口并更新最小尺寸。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：平台层不引用契约层，动作名与返回的状态按结构与 contracts/shell/window-control.ts 一致，由组装根注入；
 *       没有窗口或窗口已销毁时什么都不做；全屏时不响应最大化；最大化与全屏时只记录面板尺寸和设置最小尺寸，不改窗口大小；
 *       窗口换了一个时，面板尺寸重新从全部折叠算起；设置最小尺寸与边界的先后顺序见 setPanelExtents。
 * ------------------------------------------------------------------------
 */

import { boundsForExtents, minimumSizeFor, NO_PANEL_EXTENTS, type PanelExtents, type Rect } from './panel-bounds';

/** 被控制的窗口，BrowserWindow 满足。 */
export interface ControlledWindow {
  getBounds(): Rect;
  getNormalBounds(): Rect;
  setBounds(bounds: Rect): void;
  setMinimumSize(width: number, height: number): void;
  isDestroyed(): boolean;
  minimize(): void;
  isMaximized(): boolean;
  maximize(): void;
  unmaximize(): void;
  isFullScreen(): boolean;
  setFullScreen(flag: boolean): void;
  close(): void;
}

/** 窗口动作，与契约里的白名单一致。 */
type Action = 'minimize' | 'toggleMaximize' | 'toggleFullScreen' | 'close' | 'quit';

/** 窗口控制器。 */
export interface WindowControllerImpl {
  perform(action: Action): void;
  state(): { readonly maximized: boolean; readonly fullScreen: boolean };
  setPanelExtents(extents: PanelExtents): void;
  panelExtents(): PanelExtents;
}

/** 窗口控制器依赖的屏幕与窗口边界能力，由组装根注入。 */
export interface WindowControllerEnvironment {
  /** 包含给定矩形的屏幕的工作区。 */
  workAreaOf(rect: Rect): Rect;
  /** 把窗口设成指定边界（要修正屏幕缩放造成的取整误差）。 */
  applyBounds(window: ControlledWindow, rect: Rect): void;
}

/**
 * 创建窗口控制器。
 * @param getWindow 取当前的主窗口。
 * @param quit 退出应用。
 * @param environment 屏幕与窗口边界能力。
 * @returns 窗口控制器。
 */
export function createWindowController(getWindow: () => ControlledWindow | undefined, quit: () => void, environment: WindowControllerEnvironment): WindowControllerImpl {
  let trackedWindow: ControlledWindow | undefined;
  let extents: PanelExtents = NO_PANEL_EXTENTS;
  const alive = (): ControlledWindow | undefined => {
    const window = getWindow();
    return window !== undefined && !window.isDestroyed() ? window : undefined;
  };
  /** 窗口换了一个时，面板尺寸从全部折叠算起。 */
  const track = (window: ControlledWindow | undefined): void => {
    if (window === trackedWindow) return;
    trackedWindow = window;
    extents = NO_PANEL_EXTENTS;
  };
  return {
    perform: (action) => {
      if (action === 'quit') {
        quit();
        return;
      }
      const window = alive();
      if (window === undefined) return;
      switch (action) {
        case 'minimize':
          window.minimize();
          break;
        case 'toggleMaximize':
          if (window.isFullScreen()) break;
          if (window.isMaximized()) window.unmaximize();
          else window.maximize();
          break;
        case 'toggleFullScreen':
          window.setFullScreen(!window.isFullScreen());
          break;
        case 'close':
          window.close();
          break;
      }
    },
    state: () => {
      const window = alive();
      return { maximized: window?.isMaximized() ?? false, fullScreen: window?.isFullScreen() ?? false };
    },
    setPanelExtents: (next) => {
      const window = alive();
      track(window);
      if (window === undefined) return;
      const from = extents;
      extents = next;
      const minimum = minimumSizeFor(next);
      if (window.isMaximized() || window.isFullScreen()) {
        window.setMinimumSize(minimum.width, minimum.height);
        return;
      }
      // 先放宽最小尺寸再改边界，否则缩小窗口时会被旧的最小尺寸挡住；边界设好后再收紧。
      const previous = minimumSizeFor(from);
      window.setMinimumSize(Math.min(previous.width, minimum.width), Math.min(previous.height, minimum.height));
      const bounds = window.getBounds();
      environment.applyBounds(window, boundsForExtents(bounds, from, next, environment.workAreaOf(bounds)));
      window.setMinimumSize(minimum.width, minimum.height);
    },
    panelExtents: () => {
      track(alive());
      return extents;
    }
  };
}
