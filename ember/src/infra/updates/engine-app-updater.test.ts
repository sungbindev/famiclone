/*
 * ------------------------------------------------------------------------
 * 名称：engine-app-updater.test.ts
 * 说明：升级服务的测试：不支持升级的引擎、检查到新版本并下载好、已是最新版本、失败、检查超时、检查中不重复检查、下载好后不再响应引擎、重启安装、状态变化的通知与取消订阅。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的引擎，事件由测试手动触发；超时用假定时器。
 * ------------------------------------------------------------------------
 */

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { UpdateEngine, UpdateEngineEvents } from '../../contracts/system/app-updater';
import { EngineAppUpdater } from './engine-app-updater';

/** 假引擎：记录调用，事件由测试触发。 */
function createEngine(supported = true) {
  let events: UpdateEngineEvents | undefined;
  const engine = {
    supported,
    subscribe: vi.fn((subscriber: UpdateEngineEvents) => {
      events = subscriber;
      return () => undefined;
    }),
    check: vi.fn(),
    restart: vi.fn()
  } satisfies UpdateEngine;
  return { engine, emit: () => events as UpdateEngineEvents };
}

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.useRealTimers();
});

describe('EngineAppUpdater', () => {
  it('引擎不支持升级时状态是 unsupported，检查不会调用引擎', () => {
    const { engine } = createEngine(false);
    const updater = new EngineAppUpdater({ engine });
    expect(updater.status()).toEqual({ state: 'unsupported' });
    expect(updater.check()).toEqual({ state: 'unsupported' });
    expect(engine.check).not.toHaveBeenCalled();
    expect(engine.subscribe).not.toHaveBeenCalled();
  });

  it('开始时是 idle；检查后依次是 checking、downloading、ready（带版本号）', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    const seen: unknown[] = [];
    updater.onStatusChange((status) => seen.push(status));
    expect(updater.status()).toEqual({ state: 'idle' });

    expect(updater.check()).toEqual({ state: 'checking' });
    expect(engine.check).toHaveBeenCalledTimes(1);
    emit().available();
    emit().downloaded('0.2.0');
    expect(updater.status()).toEqual({ state: 'ready', version: '0.2.0' });
    expect(seen).toEqual([{ state: 'checking' }, { state: 'downloading' }, { state: 'ready', version: '0.2.0' }]);
  });

  it('引擎没有给出版本号时 ready 没有 version', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    updater.check();
    emit().available();
    emit().downloaded(undefined);
    expect(updater.status()).toEqual({ state: 'ready' });
  });

  it('已是最新版本时是 upToDate，之后可以再次检查', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    updater.check();
    emit().notAvailable();
    expect(updater.status()).toEqual({ state: 'upToDate' });
    expect(updater.check()).toEqual({ state: 'checking' });
    expect(engine.check).toHaveBeenCalledTimes(2);
  });

  it('检查或下载失败时是 error 并带原因，之后可以重新检查', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    updater.check();
    emit().failed('网络不通');
    expect(updater.status()).toEqual({ state: 'error', message: '网络不通' });
    updater.check();
    emit().available();
    emit().failed('下载中断');
    expect(updater.status()).toEqual({ state: 'error', message: '下载中断' });
  });

  it('引擎无法开始检查（抛出异常）时直接是 error', () => {
    const { engine } = createEngine();
    engine.check.mockImplementation(() => {
      throw new Error('没有 Update.exe');
    });
    const updater = new EngineAppUpdater({ engine });
    expect(updater.check()).toEqual({ state: 'error', message: '无法开始检查更新：没有 Update.exe' });
  });

  it('检查中、下载中不重复检查', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    updater.check();
    updater.check();
    expect(engine.check).toHaveBeenCalledTimes(1);
    emit().available();
    expect(updater.check()).toEqual({ state: 'downloading' });
    expect(engine.check).toHaveBeenCalledTimes(1);
  });

  it('检查阶段超时后是 error，可以重新检查；下载阶段不会超时', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine, checkTimeoutMs: 1000 });
    updater.check();
    vi.advanceTimersByTime(1000);
    expect(updater.status()).toEqual({ state: 'error', message: '检查更新超时，请稍后再试' });

    updater.check();
    emit().available();
    vi.advanceTimersByTime(10 * 60_000);
    expect(updater.status()).toEqual({ state: 'downloading' });
  });

  it('下载好之后不再检查，也不再响应引擎的事件', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    updater.check();
    emit().available();
    emit().downloaded('0.2.0');
    expect(updater.check()).toEqual({ state: 'ready', version: '0.2.0' });
    emit().failed('迟到的错误');
    emit().notAvailable();
    expect(updater.status()).toEqual({ state: 'ready', version: '0.2.0' });
    expect(engine.check).toHaveBeenCalledTimes(1);
  });

  it('不是由检查引起的引擎事件被忽略', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    emit().available();
    emit().failed('无关的错误');
    expect(updater.status()).toEqual({ state: 'idle' });
  });

  it('只有 ready 时才能重启安装', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    expect(updater.install()).toBe(false);
    expect(engine.restart).not.toHaveBeenCalled();
    updater.check();
    emit().available();
    emit().downloaded('0.2.0');
    expect(updater.install()).toBe(true);
    expect(engine.restart).toHaveBeenCalledTimes(1);
  });

  it('取消订阅后不再收到状态变化', () => {
    const { engine, emit } = createEngine();
    const updater = new EngineAppUpdater({ engine });
    const listener = vi.fn();
    const stop = updater.onStatusChange(listener);
    updater.check();
    stop();
    emit().notAvailable();
    expect(listener).toHaveBeenCalledTimes(1);
  });
});
