/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeStatusItems.test.tsx
 * 说明：系统栏左侧状态项与画面提示的测试：卡带名与 mapper 插件（没有信息时不显示）；画面提示的几种状态（还没读到、未开机、已断电、已暂停、运行中）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口与真实的控制器。
 * ------------------------------------------------------------------------
 */

import { act, render, screen } from '@testing-library/react';
import { describe, expect, it } from 'vitest';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';
import { CartridgeStatusItem, PluginStatusItem } from './RuntimeStatusItems';
import { ScreenOverlay } from './ScreenOverlay';

/** 接上假核心并等初始读取完成。 */
async function connect(fake: FakeRuntimeBackend): Promise<RuntimeController> {
  const controller = new RuntimeController();
  controller.attach(fake.backend, { notify: () => undefined, refreshCommands: () => undefined });
  await act(async () => settle());
  return controller;
}

describe('卡带与 mapper 插件状态项', () => {
  it('显示卡带名与 mapper 插件', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');
    const controller = await connect(fake);
    render(
      <>
        <CartridgeStatusItem controller={controller} />
        <PluginStatusItem controller={controller} />
      </>
    );
    expect(screen.getByText('nestest.nes')).toBeTruthy();
    expect(screen.getByText('NROM')).toBeTruthy();
  });

  it('没有开机时没有信息，两项都不显示（没有“—”与“无”）', async () => {
    const controller = await connect(new FakeRuntimeBackend());
    const { container } = render(
      <>
        <CartridgeStatusItem controller={controller} />
        <PluginStatusItem controller={controller} />
      </>
    );
    expect(container.textContent).toBe('');
  });

  it('整机机型用 BIOS 开机时没有卡带与 mapper，不显示', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = { ...machineStatus('on', 'running'), machine: 'sb2000' };
    const controller = await connect(fake);
    const { container } = render(
      <>
        <CartridgeStatusItem controller={controller} />
        <PluginStatusItem controller={controller} />
      </>
    );
    expect(container.textContent).toBe('');
  });

  it('核心重启后先不显示，就绪后显示新状态（旧文字不残留）', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    render(<CartridgeStatusItem controller={controller} />);
    expect(screen.getByText('a.nes')).toBeTruthy();
    act(() => fake.emitCoreStatus({ state: 'starting' }));
    expect(screen.queryByText('a.nes')).toBeNull();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\b.nes');
    act(() => fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' }));
    await act(async () => settle());
    expect(screen.getByText('b.nes')).toBeTruthy();
  });
});

describe('画面提示', () => {
  it('没有开机：“未开机”，并指明怎么开机', async () => {
    const controller = await connect(new FakeRuntimeBackend());
    render(<ScreenOverlay controller={controller} />);
    expect(screen.getByRole('status').textContent).toContain('未开机');
    expect(screen.getByRole('status').textContent).toContain('文件→选择机型');
    expect(screen.getByRole('status').textContent).toContain('文件→打开卡带');
    expect(screen.getAllByRole('term').map((term) => term.textContent)).toEqual(['学习机', '经典 FC']);
    expect(screen.getByRole('status').getAttribute('data-blank')).toBe('true');
  });

  it('断电：“已断电”；暂停：“已暂停”', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    render(<ScreenOverlay controller={controller} />);
    expect(screen.getByRole('status').textContent).toContain('已断电');
    expect(screen.getByRole('status').getAttribute('data-blank')).toBe('true');
    fake.status = machineStatus('on', 'paused', 'D:\\roms\\a.nes');
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'paused', reason: 'powerOn' });
    await act(async () => settle());
    expect(screen.getByRole('status').textContent).toBe('已暂停');
    expect(screen.getByRole('status').getAttribute('data-blank')).toBe('false');
  });

  it('运行中与还没读到状态时没有提示', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    render(<ScreenOverlay controller={controller} />);
    expect(screen.queryByRole('status')).toBeNull();
    act(() => fake.emitCoreStatus({ state: 'starting' }));
    expect(screen.queryByRole('status')).toBeNull();
  });
});
