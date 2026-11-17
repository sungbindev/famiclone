/*
 * ------------------------------------------------------------------------
 * 名称：help-window.test.ts
 * 说明：帮助文档窗口的测试：与主窗口一样隐藏系统标题栏并按主题着色系统窗口按钮，安全设置不放宽，站点不存在时报错，已打开时只聚焦，可以指定要打开的页面、窗口关闭后停止跟随系统主题。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterAll, describe, expect, it, vi } from 'vitest';

const created: Array<{ options: Record<string, unknown>; focus: ReturnType<typeof vi.fn>; handlers: Map<string, () => void> }> = [];

vi.mock('electron', () => ({
  app: { isPackaged: false },
  BrowserWindow: class {
    readonly handlers = new Map<string, () => void>();
    readonly webContents = { on: vi.fn(), setWindowOpenHandler: vi.fn() };
    constructor(readonly options: Record<string, unknown>) {
      created.push(this);
    }
    removeMenu = vi.fn();
    once = vi.fn();
    on = (name: string, handler: () => void): void => {
      this.handlers.set(name, handler);
    };
    isDestroyed = vi.fn(() => false);
    isMinimized = vi.fn(() => false);
    restore = vi.fn();
    focus = vi.fn();
    close = vi.fn();
    setBackgroundColor = vi.fn();
    setTitleBarOverlay = vi.fn();
    loadURL = vi.fn().mockResolvedValue(undefined);
  },
  Menu: { setApplicationMenu: vi.fn() },
  nativeTheme: { shouldUseDarkColors: true, themeSource: 'system', on: vi.fn(), removeListener: vi.fn() },
  screen: { getAllDisplays: () => [] },
  session: {},
  shell: { openExternal: vi.fn() }
}));

const { createHelpWindow } = await import('./help-window');
const { nativeTheme } = await import('electron');

const roots: string[] = [];

/** 建一个有首页文件的站点目录。 */
function siteDirectory(): string {
  const root = mkdtempSync(join(tmpdir(), 'ember-help-window-'));
  roots.push(root);
  writeFileSync(join(root, 'index.html'), '<html></html>');
  return root;
}

afterAll(() => {
  for (const root of roots) rmSync(root, { recursive: true, force: true });
});

/** 窗口加载过的地址，按加载顺序。 */
function urlsOf(window: unknown): unknown[] {
  return (window as { loadURL: ReturnType<typeof vi.fn> }).loadURL.mock.calls.map((call) => call[0]);
}

describe('createHelpWindow', () => {
  it('隐藏系统标题栏，Windows 与 Linux 保留按主题着色的系统窗口按钮，底色与主窗口一致', async () => {
    await createHelpWindow({ root: siteDirectory() }).open();
    const options = created[0]!.options;
    expect(options.titleBarStyle).toBe('hidden');
    expect(options.backgroundColor).toBe('#191a1b');
    if (process.platform !== 'darwin') expect(options.titleBarOverlay).toMatchObject({ color: '#191a1b', height: 35 });
  });

  it('开启沙箱与上下文隔离、关闭 Node 集成，没有预加载脚本', async () => {
    await createHelpWindow({ root: siteDirectory() }).open();
    const preferences = created[created.length - 1]!.options.webPreferences as Record<string, unknown>;
    expect(preferences).toMatchObject({ contextIsolation: true, sandbox: true, nodeIntegration: false, webSecurity: true });
    expect(preferences.preload).toBeUndefined();
  });

  it('站点目录里没有首页时报错且不创建窗口', async () => {
    const before = created.length;
    await expect(createHelpWindow({ root: join(tmpdir(), 'ember-help-window-missing') }).open()).rejects.toThrow('找不到帮助文档');
    expect(created).toHaveLength(before);
  });

  it('已经打开时只聚焦，不再创建窗口', async () => {
    const help = createHelpWindow({ root: siteDirectory() });
    await help.open();
    const window = created[created.length - 1]!;
    const before = created.length;
    await help.open();
    expect(created).toHaveLength(before);
    expect(window.focus).toHaveBeenCalledTimes(1);
  });

  it('指定页面时：新窗口直接加载该页，没指定时加载首页，已打开的窗口跳到该页且没指定时保持当前页', async () => {
    const home = createHelpWindow({ root: siteDirectory() });
    await home.open();
    expect(urlsOf(created[created.length - 1])).toEqual(['ember-help://app/']);

    const help = createHelpWindow({ root: siteDirectory() });
    await help.open('changelog');
    const window = created[created.length - 1];
    expect(urlsOf(window)).toEqual(['ember-help://app/manual/changelog.html']);
    await help.open();
    expect(urlsOf(window)).toEqual(['ember-help://app/manual/changelog.html']);

    const other = createHelpWindow({ root: siteDirectory() });
    await other.open();
    await other.open('changelog');
    expect(urlsOf(created[created.length - 1])).toEqual(['ember-help://app/', 'ember-help://app/manual/changelog.html']);
  });

  it('跟随系统主题，窗口关闭后取消监听', async () => {
    const listen = vi.mocked(nativeTheme.on);
    const unlisten = vi.mocked(nativeTheme.removeListener);
    listen.mockClear();
    unlisten.mockClear();
    await createHelpWindow({ root: siteDirectory() }).open();
    expect(listen).toHaveBeenCalledWith('updated', expect.any(Function));
    created[created.length - 1]!.handlers.get('closed')!();
    expect(unlisten).toHaveBeenCalledWith('updated', listen.mock.calls[0]![1]);
  });
});
