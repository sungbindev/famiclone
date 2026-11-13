/*
 * ------------------------------------------------------------------------
 * 名称：app-window.ts
 * 说明：主窗口：标准系统窗口，开启沙箱与上下文隔离、关闭 Node 集成，锁定导航，拒绝新窗口，拒绝全部权限请求，去掉 Electron 的默认菜单。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：窗口外框以后改为组件库的标题栏时再改本文件；渲染进程的安全设置不得放宽。
 * ------------------------------------------------------------------------
 */

import { pathToFileURL } from 'node:url';
import { BrowserWindow, Menu, session } from 'electron';

/** 窗口的最小尺寸。 */
const MIN_WIDTH = 960;
const MIN_HEIGHT = 600;
/** 窗口的初始尺寸。 */
const INITIAL_WIDTH = 1280;
const INITIAL_HEIGHT = 800;
/** 窗口的底色，界面加载完成前显示，与组件库深色主题的背景一致。 */
const BACKGROUND_COLOR = '#191a1b';

/** 主窗口的创建参数。 */
export interface AppWindowOptions {
  /** 预加载脚本路径。 */
  readonly preloadPath: string;
  /** 开发服务器地址；正式包为空。 */
  readonly devServerUrl?: string;
  /** 正式包的界面入口文件。 */
  readonly rendererFile: string;
  /** 窗口标题。 */
  readonly title: string;
  /** 按 F12 切换开发者工具（只在开发时开启）。 */
  readonly devTools?: boolean;
}

/** 应用内的地址：开发服务器或打包后的界面文件。 */
export function isAppUrl(url: string, options: Pick<AppWindowOptions, 'devServerUrl' | 'rendererFile'>): boolean {
  try {
    const target = new URL(url);
    if (options.devServerUrl) return target.origin === new URL(options.devServerUrl).origin;
    return target.protocol === 'file:' && target.pathname === pathToFileURL(options.rendererFile).pathname;
  } catch {
    return false;
  }
}

/** 去掉 Electron 的默认菜单：它的 Ctrl+W、Ctrl+R 等快捷键会抢占应用自己的命令。 */
export function removeDefaultMenu(): void {
  Menu.setApplicationMenu(null);
}

/** 拒绝全部权限请求（摄像头、通知等），以后需要时再逐项放行。 */
export function denyAllPermissions(): void {
  session.defaultSession.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
  session.defaultSession.setPermissionCheckHandler(() => false);
}

/**
 * 创建主窗口。
 * @param options 创建参数。
 * @returns 主窗口。
 */
export function createAppWindow(options: AppWindowOptions): BrowserWindow {
  const window = new BrowserWindow({
    width: INITIAL_WIDTH,
    height: INITIAL_HEIGHT,
    minWidth: MIN_WIDTH,
    minHeight: MIN_HEIGHT,
    title: options.title,
    show: false,
    backgroundColor: BACKGROUND_COLOR,
    webPreferences: {
      preload: options.preloadPath,
      contextIsolation: true,
      sandbox: true,
      nodeIntegration: false,
      webSecurity: true,
      spellcheck: false
    }
  });

  window.webContents.on('will-navigate', (event, url) => {
    if (!isAppUrl(url, options)) event.preventDefault();
  });
  window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  if (options.devTools) {
    window.webContents.on('before-input-event', (event, input) => {
      if (input.type === 'keyDown' && input.key === 'F12') {
        event.preventDefault();
        window.webContents.toggleDevTools();
      }
    });
  }
  window.once('ready-to-show', () => window.show());

  if (options.devServerUrl) void window.loadURL(options.devServerUrl);
  else void window.loadFile(options.rendererFile);
  return window;
}
