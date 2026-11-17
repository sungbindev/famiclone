/*
 * ------------------------------------------------------------------------
 * 名称：squirrel-update-engine.test.ts
 * 说明：Squirrel 升级引擎的测试：不是 Squirrel 安装的应用不支持升级且不碰 autoUpdater；支持时设置升级地址、转发 autoUpdater 的事件（版本号取自发布名，不合格时为 undefined）、检查与重启（放掉单实例锁、经启动器重新启动、退出）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的 electron 模块：autoUpdater 是事件发射器，app 记录调用。
 * ------------------------------------------------------------------------
 */

import { app, autoUpdater } from 'electron';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { createSquirrelUpdateEngine } from './squirrel-update-engine';

vi.mock('electron', async () => {
  const { EventEmitter } = await import('node:events');
  return {
    autoUpdater: Object.assign(new EventEmitter(), { setFeedURL: vi.fn(), checkForUpdates: vi.fn() }),
    app: { releaseSingleInstanceLock: vi.fn(), relaunch: vi.fn(), quit: vi.fn() }
  };
});

const FEED_URL = 'https://example.com/updates/ember/windows';
const LAUNCHER = 'C:\\Users\\sun\\AppData\\Local\\ember\\ember.exe';

beforeEach(() => {
  autoUpdater.removeAllListeners();
  vi.mocked(autoUpdater.setFeedURL).mockReset();
  vi.mocked(autoUpdater.checkForUpdates).mockReset();
  vi.mocked(app.releaseSingleInstanceLock).mockReset();
  vi.mocked(app.relaunch).mockReset();
  vi.mocked(app.quit).mockReset();
});

const electron = { autoUpdater, app };

function subscribe(engine: ReturnType<typeof createSquirrelUpdateEngine>) {
  const events = { available: vi.fn(), notAvailable: vi.fn(), downloaded: vi.fn(), failed: vi.fn() };
  engine.subscribe(events);
  return events;
}

describe('createSquirrelUpdateEngine', () => {
  it('没有启动器（不是 Squirrel 安装的应用）时不支持升级，不碰 autoUpdater，检查与重启都抛出异常', () => {
    const engine = createSquirrelUpdateEngine({ feedUrl: FEED_URL, launcher: undefined });
    expect(engine.supported).toBe(false);
    expect(electron.autoUpdater.setFeedURL).not.toHaveBeenCalled();
    expect(electron.autoUpdater.listenerCount('error')).toBe(0);
    expect(() => engine.check()).toThrow('当前运行方式不支持升级');
    expect(() => engine.restart()).toThrow('当前运行方式不支持升级');
  });

  it('支持升级时设置升级地址，检查交给 autoUpdater', () => {
    const engine = createSquirrelUpdateEngine({ feedUrl: FEED_URL, launcher: LAUNCHER });
    expect(engine.supported).toBe(true);
    expect(electron.autoUpdater.setFeedURL).toHaveBeenCalledWith({ url: FEED_URL });
    engine.check();
    expect(electron.autoUpdater.checkForUpdates).toHaveBeenCalledTimes(1);
  });

  it('把 autoUpdater 的事件转给订阅者：有新版本、没有新版本、下载好（版本号取自发布名）、失败', () => {
    const events = subscribe(createSquirrelUpdateEngine({ feedUrl: FEED_URL, launcher: LAUNCHER }));
    electron.autoUpdater.emit('update-available');
    electron.autoUpdater.emit('update-not-available');
    electron.autoUpdater.emit('update-downloaded', {}, '', '0.2.0', new Date(), FEED_URL);
    electron.autoUpdater.emit('error', new Error('网络不通'));
    expect(events.available).toHaveBeenCalledTimes(1);
    expect(events.notAvailable).toHaveBeenCalledTimes(1);
    expect(events.downloaded).toHaveBeenCalledWith('0.2.0');
    expect(events.failed).toHaveBeenCalledWith('网络不通');
  });

  it('发布名不是合格的版本号时 downloaded 没有版本号；错误没有说明时用默认文字', () => {
    const events = subscribe(createSquirrelUpdateEngine({ feedUrl: FEED_URL, launcher: LAUNCHER }));
    electron.autoUpdater.emit('update-downloaded', {}, '', '<script>', new Date(), FEED_URL);
    electron.autoUpdater.emit('update-downloaded', {}, '', '', new Date(), FEED_URL);
    electron.autoUpdater.emit('error', new Error(''));
    expect(events.downloaded.mock.calls).toEqual([[undefined], [undefined]]);
    expect(events.failed).toHaveBeenCalledWith('检查更新失败');
  });

  it('取消订阅后不再收到事件', () => {
    const engine = createSquirrelUpdateEngine({ feedUrl: FEED_URL, launcher: LAUNCHER });
    const events = { available: vi.fn(), notAvailable: vi.fn(), downloaded: vi.fn(), failed: vi.fn() };
    const stop = engine.subscribe(events);
    stop();
    electron.autoUpdater.emit('update-available');
    expect(events.available).not.toHaveBeenCalled();
  });

  it('重启：先放掉单实例锁，经启动器重新启动（不带旧的启动参数），再退出', () => {
    const order: string[] = [];
    vi.mocked(app.releaseSingleInstanceLock).mockImplementation(() => void order.push('release'));
    vi.mocked(app.relaunch).mockImplementation(() => void order.push('relaunch'));
    vi.mocked(app.quit).mockImplementation(() => void order.push('quit'));
    createSquirrelUpdateEngine({ feedUrl: FEED_URL, launcher: LAUNCHER }).restart();
    expect(order).toEqual(['release', 'relaunch', 'quit']);
    expect(electron.app.relaunch).toHaveBeenCalledWith({ execPath: LAUNCHER, args: [] });
  });
});
