/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeStatusItems.test.tsx
 * 说明：状态栏项与画面提示的测试：电源状态点的颜色含义与文字、主机型号、卡带名、mapper 插件、音量（含静音）；画面提示的四种状态（还没读到、未开机、已断电、已暂停、运行中）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的核心接口与真实的控制器。
 * ------------------------------------------------------------------------
 */

import { act, render, screen } from '@testing-library/react';
import { describe, expect, it } from 'vitest';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';
import { CartridgeStatusItem, HostStatusItem, PluginStatusItem, PowerStatusItem, VolumeStatusItem } from './RuntimeStatusItems';
import { ScreenOverlay } from './ScreenOverlay';

/** 接上假核心并等初始读取完成。 */
async function connect(fake: FakeRuntimeBackend): Promise<RuntimeController> {
  const controller = new RuntimeController();
  controller.attach(fake.backend, { addMessage: () => undefined, notify: () => undefined, refreshCommands: () => undefined });
  await act(async () => settle());
  return controller;
}

/** 电源状态项里状态点的含义。 */
function powerDot(container: HTMLElement): string | null {
  return container.querySelector('.ui-status-bar__dot')?.getAttribute('data-dot') ?? null;
}

describe('电源状态项', () => {
  it('运行中是绿色状态点加“运行中”', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    const { container } = render(<PowerStatusItem controller={controller} />);
    expect(screen.getByText('运行中')).toBeTruthy();
    expect(powerDot(container)).toBe('success');
  });

  it('已暂停是黄色状态点加“已暂停”', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'paused', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    const { container } = render(<PowerStatusItem controller={controller} />);
    expect(screen.getByText('已暂停')).toBeTruthy();
    expect(powerDot(container)).toBe('warning');
  });

  it('已断电与未开机是灰色状态点加文字', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    const { container } = render(<PowerStatusItem controller={controller} />);
    expect(screen.getByText('已断电')).toBeTruthy();
    expect(powerDot(container)).toBe('neutral');
    fake.status = machineStatus('off', 'paused');
    fake.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'eject' });
    await act(async () => settle());
    expect(screen.getByText('未开机')).toBeTruthy();
    expect(powerDot(container)).toBe('neutral');
  });

  it('状态随事件刷新：暂停变为运行', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'paused', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    const { container } = render(<PowerStatusItem controller={controller} />);
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'running', reason: 'resume' });
    await act(async () => settle());
    expect(screen.getByText('运行中')).toBeTruthy();
    expect(powerDot(container)).toBe('success');
  });

  it('核心重启后先显示“—”，就绪后显示新状态（旧文字不残留）', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    const controller = await connect(fake);
    render(<PowerStatusItem controller={controller} />);
    expect(screen.getByText('运行中')).toBeTruthy();
    act(() => fake.emitCoreStatus({ state: 'starting' }));
    expect(screen.queryByText('运行中')).toBeNull();
    expect(screen.getByText('—')).toBeTruthy();
    fake.status = machineStatus('off', 'paused');
    act(() => fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' }));
    await act(async () => settle());
    expect(screen.getByText('未开机')).toBeTruthy();
  });
});

describe('其余状态栏项', () => {
  it('主机型号、卡带名、mapper 插件', async () => {
    const fake = new FakeRuntimeBackend();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');
    const controller = await connect(fake);
    render(
      <>
        <HostStatusItem controller={controller} />
        <CartridgeStatusItem controller={controller} />
        <PluginStatusItem controller={controller} />
      </>
    );
    expect(screen.getByText('经典 FC')).toBeTruthy();
    expect(screen.getByText('nestest.nes')).toBeTruthy();
    expect(screen.getByText('NROM')).toBeTruthy();
  });

  it('没有开机时：主机型号“—”、卡带“无”、mapper 插件“—”', async () => {
    const controller = await connect(new FakeRuntimeBackend());
    render(
      <>
        <HostStatusItem controller={controller} />
        <CartridgeStatusItem controller={controller} />
        <PluginStatusItem controller={controller} />
      </>
    );
    expect(screen.getAllByText('—')).toHaveLength(2);
    expect(screen.getByText('无')).toBeTruthy();
  });

  it('音量项：显示音量百分比，静音时显示“已静音”', async () => {
    const fake = new FakeRuntimeBackend();
    fake.volume = 70;
    const controller = await connect(fake);
    render(<VolumeStatusItem controller={controller} />);
    expect(screen.getByText('音量 70%')).toBeTruthy();
    controller.toggleMute();
    await act(async () => settle());
    expect(screen.getByText('已静音')).toBeTruthy();
  });
});

describe('画面提示', () => {
  it('没有开机：“未开机”，并指明去“文件”菜单的“开机…”或“打开卡带…”', async () => {
    const controller = await connect(new FakeRuntimeBackend());
    render(<ScreenOverlay controller={controller} />);
    expect(screen.getByRole('status').textContent).toContain('未开机');
    expect(screen.getByRole('status').textContent).toContain('“文件”菜单的“开机…”或“打开卡带…”');
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
