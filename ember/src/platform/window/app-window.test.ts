/*
 * ------------------------------------------------------------------------
 * 名称：app-window.test.ts
 * 说明：主窗口的测试：应用内地址的判断、保存的窗口位置是否还在屏幕内，以及创建窗口时渲染进程的安全设置（上下文隔离、沙箱、关闭 Node 集成）、无系统边框的标题栏与导航锁定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { describe, expect, it, vi } from 'vitest';

const created: Array<{
  options: Record<string, unknown>;
  webContents: { on: ReturnType<typeof vi.fn>; setWindowOpenHandler: ReturnType<typeof vi.fn> };
  setBounds: ReturnType<typeof vi.fn>;
  once: ReturnType<typeof vi.fn>;
  maximize: ReturnType<typeof vi.fn>;
  show: ReturnType<typeof vi.fn>;
  isDestroyed: ReturnType<typeof vi.fn>;
}> = [];

vi.mock('electron', () => ({
  BrowserWindow: class {
    readonly webContents = { on: vi.fn(), setWindowOpenHandler: vi.fn(), toggleDevTools: vi.fn() };
    constructor(readonly options: Record<string, unknown>) {
      created.push(this);
    }
    once = vi.fn();
    on = vi.fn();
    static getFocusedWindow = () => null;
    show = vi.fn();
    isDestroyed = vi.fn(() => false);
    maximize = vi.fn();
    setBounds = vi.fn();
    getNormalBounds = vi.fn(() => ({ x: 100, y: 80, width: 1200, height: 800 }));
    loadURL = vi.fn().mockResolvedValue(undefined);
    loadFile = vi.fn().mockResolvedValue(undefined);
  },
  Menu: { setApplicationMenu: vi.fn() },
  nativeTheme: { shouldUseDarkColors: true, themeSource: 'system', on: vi.fn(), removeListener: vi.fn() },
  screen: { getAllDisplays: () => [{ workArea: { x: 0, y: 0, width: 1920, height: 1040 } }] },
  session: {}
}));

const { applyThemeSource, createAppWindow, isAppUrl, setExactBounds, visiblePlacement } = await import('./app-window');
const { nativeTheme } = await import('electron');

const RENDERER_FILE = resolve('app', 'renderer', 'index.html');
const RENDERER_URL = pathToFileURL(RENDERER_FILE).href;
const OPTIONS = { preloadPath: 'preload.cjs', rendererFile: RENDERER_FILE, title: 'Ember' };

/** 一个可以从外面结束的 Promise。 */
function createDeferred(): { promise: Promise<void>; resolve: () => void } {
  let resolver: (() => void) | undefined;
  const promise = new Promise<void>((resolvePromise) => {
    resolver = resolvePromise;
  });
  return { promise, resolve: () => resolver?.() };
}

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

const SCREEN = { x: 0, y: 0, width: 1920, height: 1040 };

describe('visiblePlacement', () => {
  it('没有保存过位置时返回 undefined', () => {
    expect(visiblePlacement(undefined, [SCREEN])).toBeUndefined();
    expect(visiblePlacement(null, [SCREEN])).toBeUndefined();
  });

  it('位置在屏幕内时原样沿用，只有位置', () => {
    expect(visiblePlacement({ x: 100, y: 80 }, [SCREEN])).toEqual({ x: 100, y: 80 });
  });

  it('位置已不在任何屏幕内（拔掉了副屏）或只露出一条边时返回 undefined', () => {
    expect(visiblePlacement({ x: 2500, y: 100 }, [SCREEN])).toBeUndefined();
    expect(visiblePlacement({ x: 1880, y: 100 }, [SCREEN])).toBeUndefined();
  });

  it('按最小尺寸的窗口判断是否在屏幕内：右下角还露出足够一块时有效', () => {
    expect(visiblePlacement({ x: 1830, y: 950 }, [SCREEN])).toBeUndefined();
    expect(visiblePlacement({ x: 1700, y: 900 }, [SCREEN])).toEqual({ x: 1700, y: 900 });
  });

  it('有第二块屏幕时，落在第二块屏幕内的位置有效', () => {
    const second = { x: 1920, y: 0, width: 1920, height: 1040 };
    expect(visiblePlacement({ x: 2500, y: 100 }, [SCREEN, second])).toBeDefined();
  });
});

/** 假窗口：每次设置后读回的尺寸比请求的大 growth 像素（模拟非整数倍缩放的取整）。 */
function createRoundingWindow(growth: number) {
  let bounds = { x: 0, y: 0, width: 0, height: 0 };
  const requests: Array<{ x: number; y: number; width: number; height: number }> = [];
  return {
    requests,
    setBounds: (request: { x: number; y: number; width: number; height: number }) => {
      requests.push(request);
      bounds = { ...request, width: request.width + growth, height: request.height + growth };
    },
    getNormalBounds: () => bounds
  };
}

describe('setExactBounds', () => {
  it('读回的尺寸与想要的一致时只设置一次', () => {
    const window = createRoundingWindow(0);
    setExactBounds(window, { x: 10, y: 20, width: 1280, height: 804 });
    expect(window.requests).toEqual([{ x: 10, y: 20, width: 1280, height: 804 }]);
  });

  it('读回的尺寸大了 1 像素时按差值反向再设一次，位置不变', () => {
    const window = createRoundingWindow(1);
    setExactBounds(window, { x: 10, y: 20, width: 1280, height: 804 });
    expect(window.requests).toEqual([
      { x: 10, y: 20, width: 1280, height: 804 },
      { x: 10, y: 20, width: 1279, height: 803 }
    ]);
    expect(window.getNormalBounds()).toMatchObject({ width: 1280, height: 804 });
  });

  it('始终修正不了时最多设置 3 次，不会无限循环', () => {
    const window = createRoundingWindow(1);
    // 读回值总是固定的，与请求无关。
    window.getNormalBounds = () => ({ x: 0, y: 0, width: 1, height: 1 });
    setExactBounds(window, { x: 0, y: 0, width: 1280, height: 804 });
    expect(window.requests).toHaveLength(3);
  });
});

describe('createAppWindow', () => {
  it('开启上下文隔离与沙箱、关闭 Node 集成，并使用预加载脚本', () => {
    createAppWindow(OPTIONS);
    const preferences = created[0]!.options.webPreferences as Record<string, unknown>;
    expect(preferences).toMatchObject({ preload: 'preload.cjs', contextIsolation: true, sandbox: true, nodeIntegration: false, webSecurity: true });
    expect(created[0]!.options.title).toBe('Ember');
  });

  it('隐藏系统标题栏，由界面自绘；Windows 与 Linux 保留系统窗口按钮（按主题着色）', () => {
    createAppWindow(OPTIONS);
    const options = created[1]!.options;
    expect(options.titleBarStyle).toBe('hidden');
    expect(options.frame).toBeUndefined();
    if (process.platform !== 'darwin') expect(options.titleBarOverlay).toMatchObject({ color: '#191a1b', height: 35 });
  });

  it('没有保存的位置时按最小尺寸创建，内容就绪后显示', () => {
    createAppWindow(OPTIONS);
    const window = created[created.length - 1]!;
    expect(window.options).toMatchObject({ width: 480, height: 463, minWidth: 480, minHeight: 463 });
    expect(window.options.x).toBeUndefined();
    expect(window.setBounds).not.toHaveBeenCalled();
    const ready = window.once.mock.calls.find(([name]) => name === 'ready-to-show')![1] as () => void;
    ready();
    expect(window.show).toHaveBeenCalledTimes(1);
  });

  it('有保存的位置时按位置创建，大小仍是最小尺寸，不恢复最大化', () => {
    createAppWindow({ ...OPTIONS, placement: { x: 100, y: 80 } });
    const window = created[created.length - 1]!;
    expect(window.options).toMatchObject({ x: 100, y: 80, width: 480, height: 463 });
    expect(window.setBounds).toHaveBeenCalledWith({ x: 100, y: 80, width: 480, height: 463 });
    const ready = window.once.mock.calls.find(([name]) => name === 'ready-to-show')![1] as () => void;
    ready();
    expect(window.show).toHaveBeenCalledTimes(1);
    expect(window.maximize).not.toHaveBeenCalled();
  });

  it('保存的位置不在屏幕内时用默认位置，大小是最小尺寸', () => {
    createAppWindow({ ...OPTIONS, placement: { x: 5000, y: 5000 } });
    const window = created[created.length - 1]!;
    expect(window.options.x).toBeUndefined();
    expect(window.options).toMatchObject({ width: 480, height: 463 });
    expect(window.setBounds).not.toHaveBeenCalled();
  });

  it('只放行组件库的对话框窗口（底色随主题），其他新窗口一律拒绝，并拦截跳转到应用以外的地址', () => {
    createAppWindow(OPTIONS);
    const { webContents } = created[created.length - 1]!;
    const handler = webContents.setWindowOpenHandler.mock.calls[0]![0] as (details: { url: string; frameName: string; features: string }) => { action: string; overrideBrowserWindowOptions?: Record<string, unknown> };
    expect(handler({ url: 'https://example.com/', frameName: 'ui-dialog-1', features: '' })).toEqual({ action: 'deny' });
    expect(handler({ url: 'about:blank', frameName: 'other', features: '' })).toEqual({ action: 'deny' });
    const dialog = handler({ url: 'about:blank', frameName: 'ui-dialog-1', features: 'width=640,height=400' });
    expect(dialog.action).toBe('allow');
    expect(dialog.overrideBrowserWindowOptions).toMatchObject({ width: 640, height: 400, backgroundColor: '#191a1b', modal: false });

    const navigate = webContents.on.mock.calls.find(([name]) => name === 'will-navigate')![1] as (event: { preventDefault: () => void }, url: string) => void;
    const outside = { preventDefault: vi.fn() };
    navigate(outside, 'https://example.com/');
    expect(outside.preventDefault).toHaveBeenCalled();
    const inside = { preventDefault: vi.fn() };
    navigate(inside, RENDERER_URL);
    expect(inside.preventDefault).not.toHaveBeenCalled();
  });

  it('指定了 showAfter 时内容就绪后还要等它结束才显示，成功或失败都显示', async () => {
    const deferred = createDeferred();
    createAppWindow({ ...OPTIONS, showAfter: deferred.promise });
    const waiting = created[created.length - 1]!;
    (waiting.once.mock.calls.find(([name]) => name === 'ready-to-show')![1] as () => void)();
    expect(waiting.show).not.toHaveBeenCalled();
    deferred.resolve();
    await Promise.resolve();
    await Promise.resolve();
    expect(waiting.show).toHaveBeenCalledTimes(1);

    createAppWindow({ ...OPTIONS, showAfter: Promise.reject(new Error('没有拿到')) });
    const failed = created[created.length - 1]!;
    (failed.once.mock.calls.find(([name]) => name === 'ready-to-show')![1] as () => void)();
    await Promise.resolve();
    await Promise.resolve();
    expect(failed.show).toHaveBeenCalledTimes(1);
  });

  it('等待期间窗口已经销毁就不再显示', async () => {
    createAppWindow({ ...OPTIONS, showAfter: Promise.resolve() });
    const window = created[created.length - 1]!;
    window.isDestroyed.mockReturnValue(true);
    (window.once.mock.calls.find(([name]) => name === 'ready-to-show')![1] as () => void)();
    await Promise.resolve();
    await Promise.resolve();
    expect(window.show).not.toHaveBeenCalled();
  });
});

describe('applyThemeSource', () => {
  it('把主题来源交给系统，界面与窗口底色随系统主题变化', () => {
    applyThemeSource('dark');
    expect(nativeTheme.themeSource).toBe('dark');
    applyThemeSource('system');
    expect(nativeTheme.themeSource).toBe('system');
  });
});
