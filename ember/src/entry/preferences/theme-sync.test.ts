/*
 * ------------------------------------------------------------------------
 * 名称：theme-sync.test.ts
 * 说明：主题同步的测试：核心就绪后读取并应用主题、之后的变化立即应用、不认识的取值不改动、核心重启后重新读取、读取失败不影响运行、停止后不再应用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../contracts/core-gateway/core-gateway';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import { startThemeSync, THEME_SETTING_ID } from './theme-sync';

/** 假网关：可以主动触发事件与状态变化。 */
function fakeGateway(initial: CoreStatus, theme: () => Promise<unknown>) {
  let status = initial;
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const request = vi.fn(() => theme().then((value) => ({ groups: [], settings: [{ id: THEME_SETTING_ID, value }] })));
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

const READY: CoreStatus = { state: 'ready', coreVersion: '0.1.0' };

describe('startThemeSync', () => {
  it('核心已经就绪时立即读取并应用保存的主题，first 随后结束', async () => {
    const fake = fakeGateway(READY, () => Promise.resolve('dark'));
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource });
    await sync.first;
    expect(fake.request).toHaveBeenCalledWith('preferences.list', {});
    expect(applyThemeSource).toHaveBeenCalledWith('dark');
  });

  it('核心还在启动时先不读，就绪之后才读取并应用', async () => {
    const fake = fakeGateway({ state: 'starting' }, () => Promise.resolve('light'));
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource });
    expect(fake.request).not.toHaveBeenCalled();
    fake.setStatus(READY);
    await sync.first;
    expect(applyThemeSource).toHaveBeenCalledWith('light');
  });

  it('首选项变化的事件立即应用，别的设置项与不认识的取值不改动', async () => {
    const fake = fakeGateway(READY, () => Promise.resolve('system'));
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource });
    await sync.first;
    applyThemeSource.mockClear();
    fake.emit('preferences.changed', { id: THEME_SETTING_ID, value: 'dark' });
    fake.emit('preferences.changed', { id: 'display.aspect', value: 'dark' });
    fake.emit('preferences.changed', { id: THEME_SETTING_ID, value: 'pink' });
    fake.emit('preferences.changed', { id: THEME_SETTING_ID, value: 3 });
    fake.emit('preferences.changed', null);
    fake.emit('runtime.stateChanged', { id: THEME_SETTING_ID, value: 'light' });
    expect(applyThemeSource.mock.calls).toEqual([['dark']]);
  });

  it('核心进程重启后重新读取', async () => {
    let saved = 'dark';
    const fake = fakeGateway(READY, () => Promise.resolve(saved));
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource });
    await sync.first;
    saved = 'light';
    fake.setStatus({ state: 'starting' });
    fake.setStatus(READY);
    await vi.waitFor(() => expect(applyThemeSource).toHaveBeenLastCalledWith('light'));
  });

  it('读取失败时报告错误但 first 照样结束', async () => {
    const fake = fakeGateway(READY, () => Promise.reject(new Error('核心没有响应')));
    const onError = vi.fn();
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource, onError });
    await sync.first;
    expect(onError).toHaveBeenCalledTimes(1);
    expect(applyThemeSource).not.toHaveBeenCalled();
  });

  it('读到的主题不认识时不应用', async () => {
    const fake = fakeGateway(READY, () => Promise.resolve('pink'));
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource });
    await sync.first;
    expect(applyThemeSource).not.toHaveBeenCalled();
  });

  it('停止后取消订阅、不再应用，first 也不会一直挂着', async () => {
    const fake = fakeGateway({ state: 'starting' }, () => Promise.resolve('dark'));
    const applyThemeSource = vi.fn();
    const sync = startThemeSync({ gateway: fake.gateway, applyThemeSource });
    expect(fake.listenerCount()).toBe(2);
    sync.stop();
    expect(fake.listenerCount()).toBe(0);
    await sync.first;
    fake.emit('preferences.changed', { id: THEME_SETTING_ID, value: 'dark' });
    expect(applyThemeSource).not.toHaveBeenCalled();
  });
});
