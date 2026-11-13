/*
 * ------------------------------------------------------------------------
 * 名称：app-window.test.ts
 * 说明：主窗口的测试：应用内地址的判断，以及创建窗口时渲染进程的安全设置（上下文隔离、沙箱、关闭 Node 集成）与导航锁定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { describe, expect, it, vi } from 'vitest';

const created: Array<{ options: Record<string, unknown>; webContents: { on: ReturnType<typeof vi.fn>; setWindowOpenHandler: ReturnType<typeof vi.fn> } }> = [];

vi.mock('electron', () => ({
  BrowserWindow: class {
    readonly webContents = { on: vi.fn(), setWindowOpenHandler: vi.fn(), toggleDevTools: vi.fn() };
    constructor(readonly options: Record<string, unknown>) {
      created.push(this);
    }
    once = vi.fn();
    show = vi.fn();
    loadURL = vi.fn().mockResolvedValue(undefined);
    loadFile = vi.fn().mockResolvedValue(undefined);
  },
  Menu: { setApplicationMenu: vi.fn() },
  session: {}
}));

const { createAppWindow, isAppUrl } = await import('./app-window');

const RENDERER_FILE = resolve('app', 'renderer', 'index.html');
const RENDERER_URL = pathToFileURL(RENDERER_FILE).href;
const OPTIONS = { preloadPath: 'preload.cjs', rendererFile: RENDERER_FILE, title: 'Ember' };

describe('isAppUrl', () => {
  it('正式包只认界面入口文件', () => {
    expect(isAppUrl(RENDERER_URL, OPTIONS)).toBe(true);
    expect(isAppUrl(RENDERER_URL.replace('index.html', 'other.html'), OPTIONS)).toBe(false);
    expect(isAppUrl('https://example.com/', OPTIONS)).toBe(false);
    expect(isAppUrl('不是地址', OPTIONS)).toBe(false);
  });

  it('开发时只认开发服务器的来源', () => {
    const options = { ...OPTIONS, devServerUrl: 'http://localhost:5173/' };
    expect(isAppUrl('http://localhost:5173/index.html', options)).toBe(true);
    expect(isAppUrl('http://localhost:5174/', options)).toBe(false);
  });
});

describe('createAppWindow', () => {
  it('开启上下文隔离与沙箱、关闭 Node 集成，并使用预加载脚本', () => {
    createAppWindow(OPTIONS);
    const preferences = created[0]!.options.webPreferences as Record<string, unknown>;
    expect(preferences).toMatchObject({ preload: 'preload.cjs', contextIsolation: true, sandbox: true, nodeIntegration: false, webSecurity: true });
    expect(created[0]!.options.title).toBe('Ember');
  });

  it('拒绝新窗口，并拦截跳转到应用以外的地址', () => {
    createAppWindow(OPTIONS);
    const { webContents } = created[1]!;
    const handler = webContents.setWindowOpenHandler.mock.calls[0]![0] as () => { action: string };
    expect(handler()).toEqual({ action: 'deny' });

    const navigate = webContents.on.mock.calls.find(([name]) => name === 'will-navigate')![1] as (event: { preventDefault: () => void }, url: string) => void;
    const outside = { preventDefault: vi.fn() };
    navigate(outside, 'https://example.com/');
    expect(outside.preventDefault).toHaveBeenCalled();
    const inside = { preventDefault: vi.fn() };
    navigate(inside, RENDERER_URL);
    expect(inside.preventDefault).not.toHaveBeenCalled();
  });
});
