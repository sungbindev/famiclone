/*
 * ------------------------------------------------------------------------
 * 名称：RuntimePanel.test.tsx
 * 说明：底部面板“运行”内容的测试：没有分组标题，不显示机器信息（在系统栏）、四个控制按钮的名称与各状态下的可用性、点击发出的请求、声音（静音按钮、音量滑块与数值）、不显示速度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口与真实的控制器；按钮的请求经控制器发往假核心，结果取自假核心的记录。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it } from 'vitest';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';
import { RuntimePanel } from './RuntimePanel';

/** 接上假核心并渲染面板。 */
async function setup(status = machineStatus('off', 'paused')) {
  const fake = new FakeRuntimeBackend();
  fake.status = status;
  const controller = new RuntimeController();
  controller.attach(fake.backend, { notify: () => undefined, refreshCommands: () => undefined });
  render(
    <UiProvider>
      <RuntimePanel controller={controller} />
    </UiProvider>
  );
  await act(async () => settle());
  return { fake, controller };
}

describe('面板内容', () => {
  it('不显示主机型号、卡带与 mapper 插件（都在系统栏），也没有分组标题与“状态”', async () => {
    await setup(machineStatus('on', 'running', 'D:\\roms\\nestest.nes'));
    for (const text of ['主机型号', '经典 FC', '卡带', 'nestest.nes', 'mapper 插件', 'NROM', '状态']) expect(screen.queryByText(text)).toBeNull();
    expect(screen.queryByRole('heading')).toBeNull();
  });
});

describe('控制按钮', () => {
  it('四个按钮排在同一个容器里', async () => {
    await setup();
    const buttons = ['开机', '继续', '软复位', '硬复位'].map((name) => screen.getByRole('button', { name }));
    const parent = buttons[0]?.parentElement;
    expect(buttons.every((button) => button.parentElement === parent)).toBe(true);
  });

  it('没有开机：四个按钮全部置灰', async () => {
    await setup();
    for (const name of ['开机', '继续', '软复位', '硬复位']) expect((screen.getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(true);
  });

  it('断电：开机与硬复位可用，继续与软复位置灰', async () => {
    await setup(machineStatus('off', 'paused', 'D:\\roms\\a.nes'));
    expect((screen.getByRole('button', { name: '开机' }) as HTMLButtonElement).disabled).toBe(false);
    expect((screen.getByRole('button', { name: '继续' }) as HTMLButtonElement).disabled).toBe(true);
    expect((screen.getByRole('button', { name: '软复位' }) as HTMLButtonElement).disabled).toBe(true);
    expect((screen.getByRole('button', { name: '硬复位' }) as HTMLButtonElement).disabled).toBe(false);
  });

  it('运行中：按钮名称是“断电”“暂停”，四个都可用', async () => {
    await setup(machineStatus('on', 'running', 'D:\\roms\\a.nes'));
    for (const name of ['断电', '暂停', '软复位', '硬复位']) expect((screen.getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(false);
  });

  it('点击按钮发出对应的请求，状态与按钮名称随之变化', async () => {
    const { fake } = await setup(machineStatus('on', 'running', 'D:\\roms\\a.nes'));
    await userEvent.click(screen.getByRole('button', { name: '暂停' }));
    await act(async () => settle());
    expect(fake.calls).toContain('runtime.pause {}');
    expect(screen.getByRole('button', { name: '继续' })).toBeTruthy();
    await userEvent.click(screen.getByRole('button', { name: '软复位' }));
    await userEvent.click(screen.getByRole('button', { name: '硬复位' }));
    await userEvent.click(screen.getByRole('button', { name: '断电' }));
    await act(async () => settle());
    expect(fake.calls).toEqual(expect.arrayContaining(['runtime.reset {"kind":"soft"}', 'runtime.reset {"kind":"hard"}', 'runtime.powerOff {}']));
    expect(screen.getByRole('button', { name: '开机' })).toBeTruthy();
  });
});

describe('声音', () => {
  it('有静音按钮、音量滑块与数值，没有分组标题；不显示速度', async () => {
    await setup();
    expect(screen.getByRole('button', { name: '静音' })).toBeTruthy();
    expect(screen.getByRole('slider', { name: '音量' })).toBeTruthy();
    expect(screen.getByText('100%')).toBeTruthy();
    expect(screen.queryByText('速度')).toBeNull();
    expect(screen.queryByRole('heading')).toBeNull();
  });

  it('拖动滑块调整音量，数值同步', async () => {
    const { fake } = await setup();
    fireEvent.change(screen.getByRole('slider', { name: '音量' }), { target: { value: '35' } });
    await act(async () => settle());
    expect(fake.calls).toContain('output.setVolume {"volume":35}');
    expect(screen.getByText('35%')).toBeTruthy();
  });

  it('静音按钮切换静音并记住原音量：静音时按钮是按下状态，音量数值与滑块保持', async () => {
    const { fake } = await setup();
    fireEvent.change(screen.getByRole('slider', { name: '音量' }), { target: { value: '55' } });
    await act(async () => settle());
    await userEvent.click(screen.getByRole('button', { name: '静音' }));
    await act(async () => settle());
    expect(fake.muted).toBe(true);
    expect(fake.volume).toBe(55);
    expect(screen.getByRole('button', { name: '静音' }).getAttribute('aria-pressed')).toBe('true');
    expect(screen.getByText('55%')).toBeTruthy();
    await userEvent.click(screen.getByRole('button', { name: '静音' }));
    await act(async () => settle());
    expect(fake.muted).toBe(false);
    expect(fake.volume).toBe(55);
    expect(screen.getByRole('button', { name: '静音' }).getAttribute('aria-pressed')).toBe('false');
  });
});
