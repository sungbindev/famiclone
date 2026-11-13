/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeControls.test.tsx
 * 说明：运行页开发期操作条的测试：状态文字、按钮可用性、各按钮发出的请求、事件触发重新读取状态、失败时经通知给出说明、音量与静音。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { createRef } from 'react';
import { describe, expect, it, vi } from 'vitest';
import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { RuntimeControls } from './RuntimeControls';

/** 构造状态；machine 为 undefined 表示没有卡带。 */
function status(power: 'off' | 'on', mode: 'paused' | 'running', machine?: string): RuntimeStatusResult {
  const base: RuntimeStatusResult = { power, mode, frame: 0, cycles: 0, speed: 100 };
  if (machine === undefined) return base;
  return { ...base, machine, cartridge: { path: 'D:\\roms\\nestest.nes' } as RuntimeStatusResult['cartridge'] };
}

/** 假的接口：状态由测试决定，请求记录在 calls 里。 */
function setup(initial: RuntimeStatusResult) {
  let current = initial;
  let listener: ((name: string, data: unknown) => void) | undefined;
  const state = { power: 'on', mode: 'running' } as const;
  const request = vi.fn().mockResolvedValue({ state });
  const runtime = {
    status: vi.fn(() => Promise.resolve(current)),
    powerOn: request,
    powerOff: request,
    pause: request,
    resume: request,
    reset: request
  };
  const output = { setVolume: vi.fn().mockResolvedValue({ volume: 100, muted: false }) };
  const open = vi.fn().mockResolvedValue({ canceled: true });
  const bridge = {
    core: { runtime, output } as unknown as EmberBridge['core'],
    main: { cartridges: { open } } as unknown as EmberBridge['main'],
    streams: { requestPorts: () => undefined },
    onCoreEvent: (next: (name: string, data: unknown) => void) => {
      listener = next;
      return () => {
        listener = undefined;
      };
    },
    getCoreStatus: () => Promise.reject(new Error('未使用'))
  } as EmberBridge;
  const screenElement = document.createElement('div');
  screenElement.tabIndex = 0;
  document.body.append(screenElement);
  const screenRef = createRef<HTMLDivElement>();
  (screenRef as { current: HTMLDivElement | null }).current = screenElement;
  render(
    <UiProvider>
      <RuntimeControls bridge={bridge} screenRef={screenRef} />
    </UiProvider>
  );
  return {
    runtime,
    output,
    open,
    request,
    screenElement,
    setStatus: (next: RuntimeStatusResult) => {
      current = next;
    },
    emit: (name: string) => act(() => listener?.(name, {}))
  };
}

describe('RuntimeControls：状态与可用性', () => {
  it('没有卡带时显示未装入卡带，只有打开卡带可用', async () => {
    setup(status('off', 'paused'));
    expect(await screen.findByText('未装入卡带')).toBeTruthy();
    expect((screen.getByRole('button', { name: /打开卡带/ }) as HTMLButtonElement).disabled).toBe(false);
    for (const name of ['上电', '继续', '软复位', '硬复位']) expect((screen.getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(true);
  });

  it('断电时只有上电与硬复位可用，运行中按钮显示断电、暂停', async () => {
    setup(status('off', 'paused', 'fc'));
    expect(await screen.findByText('已断电 · nestest.nes')).toBeTruthy();
    expect((screen.getByRole('button', { name: '上电' }) as HTMLButtonElement).disabled).toBe(false);
    expect((screen.getByRole('button', { name: '继续' }) as HTMLButtonElement).disabled).toBe(true);
    expect((screen.getByRole('button', { name: '软复位' }) as HTMLButtonElement).disabled).toBe(true);
    expect((screen.getByRole('button', { name: '硬复位' }) as HTMLButtonElement).disabled).toBe(false);
  });

  it('收到 runtime.stateChanged 与 cartridges.loaded 事件后重新读取状态，其他事件不读', async () => {
    const view = setup(status('off', 'paused'));
    await screen.findByText('未装入卡带');
    expect(view.runtime.status).toHaveBeenCalledTimes(1);
    view.setStatus(status('on', 'running', 'fc'));
    await view.emit('mappers.changed');
    expect(view.runtime.status).toHaveBeenCalledTimes(1);
    await view.emit('runtime.stateChanged');
    expect(await screen.findByText('运行中 · nestest.nes')).toBeTruthy();
    view.setStatus(status('on', 'paused', 'fc'));
    await view.emit('cartridges.loaded');
    expect(await screen.findByText('已暂停 · nestest.nes')).toBeTruthy();
    expect(view.runtime.status).toHaveBeenCalledTimes(3);
  });
});

describe('RuntimeControls：操作', () => {
  it('打开卡带经主进程请求 cartridges.open', async () => {
    const view = setup(status('off', 'paused'));
    await screen.findByText('未装入卡带');
    await userEvent.click(screen.getByRole('button', { name: /打开卡带/ }));
    expect(view.open).toHaveBeenCalledWith({});
  });

  it('运行中：断电发 powerOff，暂停发 pause，软硬复位发 reset', async () => {
    const view = setup(status('on', 'running', 'fc'));
    await screen.findByText('运行中 · nestest.nes');
    await userEvent.click(screen.getByRole('button', { name: '暂停' }));
    expect(view.request).toHaveBeenLastCalledWith({});
    expect(view.runtime.pause).toHaveBeenCalledTimes(1);
    await userEvent.click(screen.getByRole('button', { name: '软复位' }));
    expect(view.runtime.reset).toHaveBeenLastCalledWith({ kind: 'soft' });
    await userEvent.click(screen.getByRole('button', { name: '硬复位' }));
    expect(view.runtime.reset).toHaveBeenLastCalledWith({ kind: 'hard' });
    await userEvent.click(screen.getByRole('button', { name: '断电' }));
    expect(view.runtime.powerOff).toHaveBeenCalledWith({});
  });

  it('已暂停时按钮显示继续，发 resume；断电时上电带 resume: true', async () => {
    const paused = setup(status('on', 'paused', 'fc'));
    await screen.findByText('已暂停 · nestest.nes');
    await userEvent.click(screen.getByRole('button', { name: '继续' }));
    expect(paused.runtime.resume).toHaveBeenCalledWith({});
  });

  it('断电时上电带 resume: true', async () => {
    const view = setup(status('off', 'paused', 'fc'));
    await screen.findByText('已断电 · nestest.nes');
    await userEvent.click(screen.getByRole('button', { name: '上电' }));
    expect(view.runtime.powerOn).toHaveBeenCalledWith({ resume: true });
  });

  it('操作结束后把焦点还给画面区', async () => {
    const view = setup(status('on', 'running', 'fc'));
    await screen.findByText('运行中 · nestest.nes');
    await userEvent.click(screen.getByRole('button', { name: '软复位' }));
    await waitFor(() => expect(document.activeElement).toBe(view.screenElement));
  });

  it('请求失败时经通知给出说明', async () => {
    const view = setup(status('on', 'running', 'fc'));
    await screen.findByText('运行中 · nestest.nes');
    view.runtime.reset.mockRejectedValueOnce({ code: 'runtime.notPowered', message: '主机没有通电' });
    await userEvent.click(screen.getByRole('button', { name: '软复位' }));
    expect(await screen.findByText('操作失败：主机没有通电')).toBeTruthy();
  });
});

describe('RuntimeControls：音量', () => {
  it('拖动音量滑块发 setVolume 的 volume，勾选静音发 muted', async () => {
    const view = setup(status('on', 'running', 'fc'));
    await screen.findByText('运行中 · nestest.nes');
    fireEvent.change(screen.getByRole('slider', { name: '音量' }), { target: { value: '40' } });
    expect(view.output.setVolume).toHaveBeenLastCalledWith({ volume: 40 });
    await userEvent.click(screen.getByRole('checkbox', { name: '静音' }));
    expect(view.output.setVolume).toHaveBeenLastCalledWith({ muted: true });
  });

  it('音量请求失败时经通知给出说明', async () => {
    const view = setup(status('on', 'running', 'fc'));
    await screen.findByText('运行中 · nestest.nes');
    view.output.setVolume.mockRejectedValueOnce({ code: 'core.invalidParams', message: '参数不对' });
    fireEvent.change(screen.getByRole('slider', { name: '音量' }), { target: { value: '10' } });
    expect(await screen.findByText('操作失败：参数不对')).toBeTruthy();
  });
});
