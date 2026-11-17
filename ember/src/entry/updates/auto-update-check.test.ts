/*
 * ------------------------------------------------------------------------
 * 名称：auto-update-check.test.ts
 * 说明：自动检查更新的测试：核心就绪后读到开启就等延迟再检查一次、关闭时不检查、等待期间关闭会取消、开启前设置变化后才开启会安排检查、每次运行只检查一次、不支持升级时什么都不做、读取失败不影响运行、停止后不再检查。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假网关与假定时器；网关的假造方式同主题同步的测试。
 * ------------------------------------------------------------------------
 */

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../contracts/core-gateway/core-gateway';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { UpdateStatus } from '../../contracts/system/generated/system';
import { AUTO_CHECK_SETTING_ID, startAutoUpdateCheck } from './auto-update-check';

const READY: CoreStatus = { state: 'ready', coreVersion: '0.1.0' };
const DELAY_MS = 10_000;

/** 假网关：可以主动触发事件与状态变化。 */
function fakeGateway(initial: CoreStatus, list: () => Promise<unknown>) {
  let status = initial;
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const request = vi.fn(() => list());
  const gateway = {
    request,
    onEvent: (listener: (name: string, data: unknown) => void) => {
      eventListeners.add(listener);
      return () => eventListeners.delete(listener);
    },
    onStatusChange: (listener: (status: CoreStatus) => void) => {
      statusListeners.add(listener);
      return () => statusListeners.delete(listener);
    },
    status: () => status
  } as unknown as CoreGateway;
  return {
    gateway,
    request,
    emit: (name: string, data: unknown) => eventListeners.forEach((listener) => listener(name, data)),
    setStatus: (next: CoreStatus) => {
      status = next;
      statusListeners.forEach((listener) => listener(next));
    },
    listenerCount: () => eventListeners.size + statusListeners.size
  };
}

/** 列出首选项的结果。 */
function listed(value: unknown): Promise<unknown> {
  return Promise.resolve({ groups: [], settings: [{ id: AUTO_CHECK_SETTING_ID, value }] });
}

function fakeUpdater(state: UpdateStatus['state'] = 'idle') {
  return { status: vi.fn((): UpdateStatus => ({ state })), check: vi.fn((): UpdateStatus => ({ state: 'checking' })) };
}

/** 让已排队的 Promise 回调都跑完。 */
async function settle(): Promise<void> {
  await vi.advanceTimersByTimeAsync(0);
}

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.useRealTimers();
});

describe('startAutoUpdateCheck', () => {
  it('核心就绪后读到开启，等延迟之后检查一次', async () => {
    const fake = fakeGateway(READY, () => listed(true));
    const updater = fakeUpdater();
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await settle();
    expect(fake.request).toHaveBeenCalledWith('preferences.list', {});
    expect(updater.check).not.toHaveBeenCalled();
    await vi.advanceTimersByTimeAsync(DELAY_MS);
    expect(updater.check).toHaveBeenCalledTimes(1);
  });

  it('核心还在启动时先不读，就绪之后才读取并安排检查', async () => {
    const fake = fakeGateway({ state: 'starting' }, () => listed(true));
    const updater = fakeUpdater();
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    expect(fake.request).not.toHaveBeenCalled();
    fake.setStatus(READY);
    await vi.advanceTimersByTimeAsync(DELAY_MS);
    expect(updater.check).toHaveBeenCalledTimes(1);
  });

  it('关闭时不检查', async () => {
    const fake = fakeGateway(READY, () => listed(false));
    const updater = fakeUpdater();
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await vi.advanceTimersByTimeAsync(DELAY_MS * 2);
    expect(updater.check).not.toHaveBeenCalled();
  });

  it('等待期间关闭会取消；之后重新开启会再安排', async () => {
    const fake = fakeGateway(READY, () => listed(true));
    const updater = fakeUpdater();
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await settle();
    fake.emit('preferences.changed', { id: AUTO_CHECK_SETTING_ID, value: false });
    await vi.advanceTimersByTimeAsync(DELAY_MS * 2);
    expect(updater.check).not.toHaveBeenCalled();
    fake.emit('preferences.changed', { id: AUTO_CHECK_SETTING_ID, value: true });
    await vi.advanceTimersByTimeAsync(DELAY_MS);
    expect(updater.check).toHaveBeenCalledTimes(1);
  });

  it('开始时关闭、之后才开启也会检查；别的设置项与不合格的取值被忽略', async () => {
    const fake = fakeGateway(READY, () => listed(false));
    const updater = fakeUpdater();
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await settle();
    fake.emit('preferences.changed', { id: 'display.aspect', value: true });
    fake.emit('preferences.changed', { id: AUTO_CHECK_SETTING_ID, value: 'yes' });
    fake.emit('preferences.changed', null);
    fake.emit('runtime.stateChanged', { id: AUTO_CHECK_SETTING_ID, value: true });
    await vi.advanceTimersByTimeAsync(DELAY_MS * 2);
    expect(updater.check).not.toHaveBeenCalled();
    fake.emit('preferences.changed', { id: AUTO_CHECK_SETTING_ID, value: true });
    await vi.advanceTimersByTimeAsync(DELAY_MS);
    expect(updater.check).toHaveBeenCalledTimes(1);
  });

  it('每次运行只自动检查一次：核心重启后重新读取也不再检查', async () => {
    const fake = fakeGateway(READY, () => listed(true));
    const updater = fakeUpdater();
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await vi.advanceTimersByTimeAsync(DELAY_MS);
    fake.setStatus({ state: 'starting' });
    fake.setStatus(READY);
    fake.emit('preferences.changed', { id: AUTO_CHECK_SETTING_ID, value: true });
    await vi.advanceTimersByTimeAsync(DELAY_MS * 2);
    expect(updater.check).toHaveBeenCalledTimes(1);
  });

  it('不支持升级时什么都不做', async () => {
    const fake = fakeGateway(READY, () => listed(true));
    const updater = fakeUpdater('unsupported');
    startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await vi.advanceTimersByTimeAsync(DELAY_MS * 2);
    expect(fake.request).not.toHaveBeenCalled();
    expect(fake.listenerCount()).toBe(0);
    expect(updater.check).not.toHaveBeenCalled();
  });

  it('读取首选项失败时交给 onError，不影响运行', async () => {
    const fake = fakeGateway(READY, () => Promise.reject(new Error('核心不可用')));
    const onError = vi.fn();
    startAutoUpdateCheck({ gateway: fake.gateway, updater: fakeUpdater(), delayMs: DELAY_MS, onError });
    await settle();
    expect(onError).toHaveBeenCalledTimes(1);
  });

  it('停止后取消还没到时间的检查并取消订阅', async () => {
    const fake = fakeGateway(READY, () => listed(true));
    const updater = fakeUpdater();
    const check = startAutoUpdateCheck({ gateway: fake.gateway, updater, delayMs: DELAY_MS });
    await settle();
    check.stop();
    await vi.advanceTimersByTimeAsync(DELAY_MS * 2);
    expect(updater.check).not.toHaveBeenCalled();
    expect(fake.listenerCount()).toBe(0);
  });
});
