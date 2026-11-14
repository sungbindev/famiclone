/*
 * ------------------------------------------------------------------------
 * 名称：app-window.ts
 * 说明：主窗口：隐藏系统标题栏并保留系统窗口按钮（按主题着色），开启沙箱与上下文隔离、关闭 Node 集成，锁定导航，拒绝新窗口，拒绝全部权限请求；
 *       恢复与读取窗口的位置和大小，去掉 Electron 的默认菜单，系统主题变化时同步窗口底色与系统按钮区的颜色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：标题栏高度与颜色取自组件库的 --title-bar-height 与深浅主题的背景色，修改时须同步；渲染进程的安全设置不得放宽。
 *       平台层不引用契约与 shared，窗口位置的类型按结构与 shared/shell-state 里的 WindowPlacement 一致。
 * ------------------------------------------------------------------------
 */

import { pathToFileURL } from 'node:url';
import { BrowserWindow, Menu, nativeTheme, screen, session } from 'electron';

/** 实际使用的主题。 */
export type ResolvedTheme = 'dark' | 'light';

/** 标题栏高度（组件库 --title-bar-height），系统按钮区少 1 像素露出底边线。 */
const TITLE_BAR_HEIGHT = 36;
/** 窗口的最小尺寸。 */
const MIN_WIDTH = 960;
const MIN_HEIGHT = 600;
/** 窗口的初始尺寸。 */
const INITIAL_WIDTH = 1280;
const INITIAL_HEIGHT = 800;
/** 窗口至少要有这么大一块落在某个屏幕的工作区内，否则恢复为默认位置。 */
const MIN_VISIBLE = 100;

/** 各主题的窗口颜色：background 是标题栏与窗口底色，symbol 是系统按钮上的符号色，与组件库深浅主题一致。 */
const WINDOW_COLORS: Readonly<Record<ResolvedTheme, { background: string; symbol: string }>> = {
  dark: { background: '#191a1b', symbol: '#8c8c8c' },
  light: { background: '#fafafd', symbol: '#606060' }
};

/** 窗口的位置与大小（非最大化时的边界）以及是否最大化。 */
export interface WindowPlacement {
  readonly x: number;
  readonly y: number;
  readonly width: number;
  readonly height: number;
  readonly maximized: boolean;
}

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
  /** 上次保存的位置与大小。 */
  readonly placement?: WindowPlacement | null;
  /** 按 F12 切换开发者工具（只在开发时开启）。 */
  readonly devTools?: boolean;
}

/** 两个矩形重叠部分的宽与高。 */
function overlap(a: Electron.Rectangle, b: Electron.Rectangle): { width: number; height: number } {
  return {
    width: Math.min(a.x + a.width, b.x + b.width) - Math.max(a.x, b.x),
    height: Math.min(a.y + a.height, b.y + b.height) - Math.max(a.y, b.y)
  };
}

/**
 * 保存的位置仍有足够部分在某个屏幕内时沿用（尺寸不小于窗口的最小尺寸），否则返回 undefined（用默认位置）。
 * @param placement 保存的位置。
 * @param workAreas 各屏幕的工作区。
 */
export function visiblePlacement(placement: WindowPlacement | null | undefined, workAreas: readonly Electron.Rectangle[]): WindowPlacement | undefined {
  if (placement === undefined || placement === null) return undefined;
  const bounds = { x: placement.x, y: placement.y, width: Math.max(placement.width, MIN_WIDTH), height: Math.max(placement.height, MIN_HEIGHT) };
  const visible = workAreas.some((area) => {
    const size = overlap(bounds, area);
    return size.width >= MIN_VISIBLE && size.height >= MIN_VISIBLE;
  });
  return visible ? { ...bounds, maximized: placement.maximized } : undefined;
}

/** 系统当前实际使用的主题。 */
export function resolvedTheme(): ResolvedTheme {
  return nativeTheme.shouldUseDarkColors ? 'dark' : 'light';
}

/** 系统标题栏按钮区的样式。 */
function titleBarOverlay(theme: ResolvedTheme): Electron.TitleBarOverlayOptions {
  const colors = WINDOW_COLORS[theme];
  return { color: colors.background, symbolColor: colors.symbol, height: TITLE_BAR_HEIGHT - 1 };
}

/** 修正窗口尺寸取整误差的最多次数。 */
const BOUNDS_FIX_ATTEMPTS = 3;

/**
 * 把窗口设成指定的边界。屏幕缩放不是整数倍时，设置后读回的尺寸会差 1 至 2 像素（如 804 变成 805），
 * 不修正的话每次启动都会比上次大一点，所以按差值反向再设一次，直到读回的与想要的一致。
 * @param window 窗口。
 * @param target 想要的边界。
 */
export function setExactBounds(window: Pick<BrowserWindow, 'setBounds' | 'getNormalBounds'>, target: Electron.Rectangle): void {
  let request = target;
  for (let attempt = 0; attempt < BOUNDS_FIX_ATTEMPTS; attempt += 1) {
    window.setBounds(request);
    const actual = window.getNormalBounds();
    const widthError = target.width - actual.width;
    const heightError = target.height - actual.height;
    if (widthError === 0 && heightError === 0) return;
    request = { ...request, width: request.width + widthError, height: request.height + heightError };
  }
}

/** 去掉 Electron 的默认菜单：它的 Ctrl+W、Ctrl+R 等快捷键会抢占外壳命令。 */
export function removeDefaultMenu(): void {
  Menu.setApplicationMenu(null);
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
  const placement = visiblePlacement(
    options.placement,
    screen.getAllDisplays().map((display) => display.workArea)
  );
  const theme = resolvedTheme();
  const window = new BrowserWindow({
    width: placement?.width ?? INITIAL_WIDTH,
    height: placement?.height ?? INITIAL_HEIGHT,
    ...(placement ? { x: placement.x, y: placement.y } : {}),
    minWidth: MIN_WIDTH,
    minHeight: MIN_HEIGHT,
    title: options.title,
    show: false,
    backgroundColor: WINDOW_COLORS[theme].background,
    titleBarStyle: 'hidden',
    ...(process.platform === 'darwin' ? {} : { titleBarOverlay: titleBarOverlay(theme) }),
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
  // 屏幕缩放不是整数倍时，创建参数里的尺寸换算后会差几个像素，所以创建后再按保存的边界精确设置一次。
  if (placement) setExactBounds(window, { x: placement.x, y: placement.y, width: placement.width, height: placement.height });
  if (options.devTools) {
    window.webContents.on('before-input-event', (event, input) => {
      if (input.type === 'keyDown' && input.key === 'F12') {
        event.preventDefault();
        window.webContents.toggleDevTools();
      }
    });
  }
  // maximize 会同时显示窗口，所以放在内容就绪后。
  window.once('ready-to-show', () => (placement?.maximized ? window.maximize() : window.show()));

  if (options.devServerUrl) void window.loadURL(options.devServerUrl);
  else void window.loadFile(options.rendererFile);
  return window;
}

/**
 * 系统主题变化时同步窗口底色与系统按钮区的颜色。
 * @param window 主窗口。
 * @returns 取消监听的函数。
 */
export function followSystemTheme(window: BrowserWindow): () => void {
  const apply = (): void => {
    if (window.isDestroyed()) return;
    const theme = resolvedTheme();
    window.setBackgroundColor(WINDOW_COLORS[theme].background);
    if (process.platform !== 'darwin') window.setTitleBarOverlay(titleBarOverlay(theme));
  };
  nativeTheme.on('updated', apply);
  return () => nativeTheme.removeListener('updated', apply);
}
