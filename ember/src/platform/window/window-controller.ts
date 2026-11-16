/*
 * ------------------------------------------------------------------------
 * 名称：window-controller.ts
 * 说明：窗口动作的执行：最小化、最大化与还原、全屏与退出全屏、置顶与取消置顶、关闭窗口、退出应用，并读出窗口的最大化、全屏与置顶状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：平台层不引用契约层，动作名与返回的状态按结构与 contracts/shell/window-control.ts 一致，由组装根注入；
 *       没有窗口或窗口已销毁时什么都不做；全屏时不响应最大化。
 * ------------------------------------------------------------------------
 */

/** 被控制的窗口，BrowserWindow 满足。 */
export interface ControlledWindow {
  isDestroyed(): boolean;
  minimize(): void;
  isMaximized(): boolean;
  maximize(): void;
  unmaximize(): void;
  isFullScreen(): boolean;
  setFullScreen(flag: boolean): void;
  isAlwaysOnTop(): boolean;
  setAlwaysOnTop(flag: boolean): void;
  close(): void;
}

/** 窗口动作，与契约里的白名单一致。 */
type Action = 'minimize' | 'toggleMaximize' | 'toggleFullScreen' | 'toggleAlwaysOnTop' | 'close' | 'quit';

/** 窗口控制器。 */
export interface WindowControllerImpl {
  perform(action: Action): void;
  state(): { readonly maximized: boolean; readonly fullScreen: boolean; readonly alwaysOnTop: boolean };
}

/**
 * 创建窗口控制器。
 * @param getWindow 取当前的主窗口。
 * @param quit 退出应用。
 * @returns 窗口控制器。
 */
export function createWindowController(getWindow: () => ControlledWindow | undefined, quit: () => void): WindowControllerImpl {
  const alive = (): ControlledWindow | undefined => {
    const window = getWindow();
    return window !== undefined && !window.isDestroyed() ? window : undefined;
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
        case 'toggleAlwaysOnTop':
          window.setAlwaysOnTop(!window.isAlwaysOnTop());
          break;
        case 'close':
          window.close();
          break;
      }
    },
    state: () => {
      const window = alive();
      return { maximized: window?.isMaximized() ?? false, fullScreen: window?.isFullScreen() ?? false, alwaysOnTop: window?.isAlwaysOnTop() ?? false };
    }
  };
}
