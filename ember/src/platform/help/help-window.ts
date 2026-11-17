/*
 * ------------------------------------------------------------------------
 * 名称：help-window.ts
 * 说明：帮助文档窗口：独立于主窗口的系统窗口，加载应用自带的帮助文档站点；只有一个，已经打开时提到最前。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：窗口没有预加载脚本、开启沙箱与上下文隔离、关闭 Node 集成，导航锁定在站点内；站点里的外部链接（http、https）交给系统浏览器，其余的丢弃；
 *       站点来自 ember-help:// 协议（见 help-protocol.ts），窗口只显示页面，不能调用主进程的任何请求；
 *       与主窗口一样隐藏系统标题栏、保留系统窗口按钮（macOS 是红绿灯），标题栏由站点页面自绘，窗口底色与按钮区颜色随主题变化（取值与主窗口共用）。
 * ------------------------------------------------------------------------
 */

import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { app, BrowserWindow, shell } from 'electron';
import { followSystemTheme, resolvedTheme, titleBarOverlay, WINDOW_COLORS } from '../window/app-window';
import { HELP_PAGE_URLS, HELP_START_URL, isHelpUrl, isOpenableExternalUrl, type HelpPageName } from './help-site';

/** 窗口的初始尺寸与最小尺寸。 */
const WINDOW_SIZE = { width: 1100, height: 760, minWidth: 640, minHeight: 420 } as const;

/** 用户可见的文字。 */
const TEXT = {
  title: 'Ember 帮助文档',
  missing: '找不到帮助文档，请重新安装 Ember；从源码运行时先运行 npm run help:build'
} as const;

/** 帮助文档窗口的创建参数。 */
export interface HelpWindowOptions {
  /** 站点目录。 */
  readonly root: string;
  /** 窗口图标；不传时用可执行文件的图标。 */
  readonly icon?: string;
}

/** 帮助文档窗口。 */
export interface HelpWindowHandle {
  /** 打开窗口；给了 page 时显示该页（已经打开的窗口也跳到该页），没给时新窗口显示首页、已打开的窗口保持当前页。 */
  open(page?: HelpPageName): Promise<void>;
  close(): void;
}

/**
 * 创建帮助文档窗口的管理器；窗口在第一次 open 时才创建。
 * @param options 创建参数。
 */
export function createHelpWindow(options: HelpWindowOptions): HelpWindowHandle {
  let window: BrowserWindow | undefined;

  const create = (startUrl: string): BrowserWindow => {
    const theme = resolvedTheme();
    const created = new BrowserWindow({
      ...WINDOW_SIZE,
      title: TEXT.title,
      ...(options.icon ? { icon: options.icon } : {}),
      show: false,
      autoHideMenuBar: true,
      backgroundColor: WINDOW_COLORS[theme].background,
      titleBarStyle: 'hidden',
      ...(process.platform === 'darwin' ? {} : { titleBarOverlay: titleBarOverlay(theme) }),
      webPreferences: { contextIsolation: true, sandbox: true, nodeIntegration: false, webSecurity: true, spellcheck: false, devTools: !app.isPackaged }
    });
    created.removeMenu();
    created.webContents.setWindowOpenHandler(({ url }) => {
      if (isOpenableExternalUrl(url)) void shell.openExternal(url);
      return { action: 'deny' };
    });
    created.webContents.on('will-navigate', (event, url) => {
      if (isHelpUrl(url)) return;
      event.preventDefault();
      if (isOpenableExternalUrl(url)) void shell.openExternal(url);
    });
    created.once('ready-to-show', () => created.show());
    const stopFollowingTheme = followSystemTheme(created);
    created.on('closed', () => {
      stopFollowingTheme();
      if (window === created) window = undefined;
    });
    void created.loadURL(startUrl);
    return created;
  };

  return {
    open: async (page) => {
      if (!existsSync(join(options.root, 'index.html'))) throw new Error(TEXT.missing);
      const pageUrl = page === undefined ? undefined : HELP_PAGE_URLS[page];
      if (window !== undefined && !window.isDestroyed()) {
        if (window.isMinimized()) window.restore();
        if (pageUrl !== undefined) void window.loadURL(pageUrl);
        window.focus();
        return;
      }
      window = create(pageUrl ?? HELP_START_URL);
    },
    close: () => {
      if (window !== undefined && !window.isDestroyed()) window.close();
    }
  };
}
