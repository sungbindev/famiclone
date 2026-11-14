/*
 * ------------------------------------------------------------------------
 * 名称：use-display-settings.test.tsx
 * 说明：运行页显示设置钩子的测试：读取画面比例、缩放方式与“显示全部 240 行”，在别处修改（preferences.changed）后更新，不认识的取值被忽略，核心重启后重新读取，读取失败时用默认值。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { act, renderHook, waitFor } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { applyDisplaySetting, DEFAULT_DISPLAY_SETTINGS, useDisplaySettings } from './use-display-settings';

/** 假预加载接口。 */
function fakeBridge(list: () => Promise<unknown>) {
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const listFn = vi.fn(list);
  const bridge = {
    core: { preferences: { list: listFn } },
    onCoreEvent: (listener: (name: string, data: unknown) => void) => {
      eventListeners.add(listener);
      return () => eventListeners.delete(listener);
    },
    onCoreStatusChange: (listener: (status: CoreStatus) => void) => {
      statusListeners.add(listener);
      return () => statusListeners.delete(listener);
    }
  } as unknown as EmberBridge;
  return {
    bridge,
    listFn,
    emit: (name: string, data: unknown) => act(() => eventListeners.forEach((listener) => listener(name, data))),
    setStatus: (status: CoreStatus) => act(() => statusListeners.forEach((listener) => listener(status)))
  };
}

const LISTED = { groups: [], settings: [{ id: 'display.aspect', value: '4:3' }, { id: 'display.scaling', value: 'integer' }, { id: 'display.showAllLines', value: false }, { id: 'sound.volume', value: 30 }] };

describe('applyDisplaySetting', () => {
  it('合并三个显示设置项，其他设置项与不认识的取值原样返回', () => {
    expect(applyDisplaySetting(DEFAULT_DISPLAY_SETTINGS, 'display.aspect', '4:3').aspect).toBe('4:3');
    expect(applyDisplaySetting(DEFAULT_DISPLAY_SETTINGS, 'display.scaling', 'integer').scaling).toBe('integer');
    expect(applyDisplaySetting(DEFAULT_DISPLAY_SETTINGS, 'display.showAllLines', false).showAllLines).toBe(false);
    expect(applyDisplaySetting(DEFAULT_DISPLAY_SETTINGS, 'display.aspect', '16:9')).toBe(DEFAULT_DISPLAY_SETTINGS);
    expect(applyDisplaySetting(DEFAULT_DISPLAY_SETTINGS, 'display.showAllLines', 'no')).toBe(DEFAULT_DISPLAY_SETTINGS);
    expect(applyDisplaySetting(DEFAULT_DISPLAY_SETTINGS, 'sound.volume', 5)).toBe(DEFAULT_DISPLAY_SETTINGS);
  });
});

describe('useDisplaySettings', () => {
  it('读到之前用默认值（8:7、适应窗口、显示全部行），读到后用核心给出的取值', async () => {
    const { bridge } = fakeBridge(() => Promise.resolve(LISTED));
    const { result } = renderHook(() => useDisplaySettings(bridge));
    expect(result.current).toEqual({ aspect: '8:7', scaling: 'fit', showAllLines: true });
    await waitFor(() => expect(result.current).toEqual({ aspect: '4:3', scaling: 'integer', showAllLines: false }));
  });

  it('在别处修改后立即更新，别的设置项与别的事件不影响', async () => {
    const page = fakeBridge(() => Promise.resolve(LISTED));
    const { result } = renderHook(() => useDisplaySettings(page.bridge));
    await waitFor(() => expect(result.current.aspect).toBe('4:3'));
    page.emit('preferences.changed', { id: 'display.showAllLines', value: true });
    expect(result.current.showAllLines).toBe(true);
    page.emit('preferences.changed', { id: 'sound.volume', value: 10 });
    page.emit('runtime.stateChanged', { id: 'display.aspect', value: '8:7' });
    page.emit('preferences.changed', null);
    expect(result.current).toEqual({ aspect: '4:3', scaling: 'integer', showAllLines: true });
  });

  it('核心重启后重新读取', async () => {
    const page = fakeBridge(() => Promise.resolve(LISTED));
    renderHook(() => useDisplaySettings(page.bridge));
    await waitFor(() => expect(page.listFn).toHaveBeenCalledTimes(1));
    page.setStatus({ state: 'starting' });
    expect(page.listFn).toHaveBeenCalledTimes(1);
    page.setStatus({ state: 'ready', coreVersion: '0.1.0' });
    await waitFor(() => expect(page.listFn).toHaveBeenCalledTimes(2));
  });

  it('读取失败时继续用默认值', async () => {
    const page = fakeBridge(() => Promise.reject(new Error('核心未启动')));
    const { result } = renderHook(() => useDisplaySettings(page.bridge));
    await waitFor(() => expect(page.listFn).toHaveBeenCalled());
    expect(result.current).toEqual(DEFAULT_DISPLAY_SETTINGS);
  });
});
