/*
 * ------------------------------------------------------------------------
 * 名称：help-window.ts
 * 说明：帮助文档窗口：独立于主窗口的系统窗口，加载应用自带的帮助文档站点；只有一个，已经打开时提到最前。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：窗口没有预加载脚本、开启沙箱与上下文隔离、关闭 Node 集成，导航锁定在站点内；站点里的外部链接（http、https）交给系统浏览器，其余的丢弃；
 *       站点来自 ember-help:// 协议（见 help-protocol.ts），窗口只显示页面，不能调用主进程的任何请求；底色跟随系统主题，与站点的深浅主题底色一致。
 * ------------------------------------------------------------------------
 */

import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { app, BrowserWindow, nativeTheme, shell } from 'electron';
import { HELP_START_URL, isHelpUrl, isOpenableExternalUrl } from './help-site';

/** 窗口的初始尺寸与最小尺寸。 */
const WINDOW_SIZE = { width: 1100, height: 760, minWidth: 640, minHeight: 420 } as const;
/** 站点深浅主题的页面底色，窗口底色与它一致，避免加载时闪一下。 */
const BACKGROUNDS = { dark: '#1b1b1f', light: '#ffffff' } as const;

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
  open(): Promise<void>;
  close(): void;
}

/**
 * 创建帮助文档窗口的管理器；窗口在第一次 open 时才创建。
 * @param options 创建参数。
 */
export function createHelpWindow(options: HelpWindowOptions): HelpWindowHandle {
  let window: BrowserWindow | undefined;

  const create = (): BrowserWindow => {
    const created = new BrowserWindow({
      ...WINDOW_SIZE,
      title: TEXT.title,
      ...(options.icon ? { icon: options.icon } : {}),
      show: false,
      autoHideMenuBar: true,
      backgroundColor: nativeTheme.shouldUseDarkColors ? BACKGROUNDS.dark : BACKGROUNDS.light,
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
    created.on('closed', () => {
      if (window === created) window = undefined;
    });
    void created.loadURL(HELP_START_URL);
    return created;
  };

  return {
    open: async () => {
      if (!existsSync(join(options.root, 'index.html'))) throw new Error(TEXT.missing);
      if (window !== undefined && !window.isDestroyed()) {
        if (window.isMinimized()) window.restore();
        window.focus();
        return;
      }
      window = create();
    },
    close: () => {
      if (window !== undefined && !window.isDestroyed()) window.close();
    }
  };
}
