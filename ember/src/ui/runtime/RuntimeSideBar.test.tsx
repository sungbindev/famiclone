/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeSideBar.test.tsx
 * 说明：运行页边栏的测试：机器组（主机型号、卡带、mapper 插件、状态；没有卡带显示“无”）、控制组（四个按钮的名称与各状态下的可用性、点击发出的请求）、声音组（静音按钮、音量滑块与数值）、不显示速度。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的核心接口与真实的控制器；按钮的请求经控制器发往假核心，结果取自假核心的记录。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render, screen, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it } from 'vitest';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';
import { RuntimeSideBar } from './RuntimeSideBar';

/** 接上假核心并渲染边栏。 */
async function setup(status = machineStatus('off', 'paused')) {
  const fake = new FakeRuntimeBackend();
  fake.status = status;
  const controller = new RuntimeController();
  controller.attach(fake.backend, { addMessage: () => undefined, notify: () => undefined, refreshCommands: () => undefined });
  render(
    <UiProvider>
      <RuntimeSideBar controller={controller} />
    </UiProvider>
  );
  await act(async () => settle());
  return { fake, controller };
}

/** 机器组里某一行的值。 */
function fact(label: string): string {
  const row = screen.getByText(label, { selector: 'dt' }).parentElement as HTMLElement;
  return within(row).getByText((_text, element) => element?.tagName === 'DD').textContent ?? '';
}

describe('机器组', () => {
  it('没有开机：主机型号“—”、卡带“无”、mapper 插件“—”、状态“未开机”', async () => {
    await setup();
    expect(screen.getByRole('heading', { name: '机器' })).toBeTruthy();
    expect([fact('主机型号'), fact('卡带'), fact('mapper 插件'), fact('状态')]).toEqual(['—', '无', '—', '未开机']);
  });

  it('运行中：显示主机型号、卡带文件名、匹配的插件与状态', async () => {
    await setup(machineStatus('on', 'running', 'D:\\roms\\nestest.nes'));
    expect([fact('主机型号'), fact('卡带'), fact('mapper 插件'), fact('状态')]).toEqual(['经典 FC', 'nestest.nes', 'NROM', '运行中']);
  });

  it('断电与暂停时状态文字随之变化', async () => {
    const { fake, controller } = await setup(machineStatus('on', 'running', 'D:\\roms\\a.nes'));
    fake.status = machineStatus('on', 'paused', 'D:\\roms\\a.nes');
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'paused', reason: 'pause' });
    await act(async () => settle());
    expect(fact('状态')).toBe('已暂停');
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    fake.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'powerOff' });
    await act(async () => settle());
    expect(fact('状态')).toBe('已断电');
    expect(controller.machineState()).toBe('off');
  });
});

describe('控制组', () => {
  it('没有开机：四个按钮全部置灰', async () => {
    await setup();
    for (const name of ['上电', '继续', '软复位', '硬复位']) expect((screen.getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(true);
  });

  it('断电：上电与硬复位可用，继续与软复位置灰', async () => {
    await setup(machineStatus('off', 'paused', 'D:\\roms\\a.nes'));
    expect((screen.getByRole('button', { name: '上电' }) as HTMLButtonElement).disabled).toBe(false);
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
    expect(screen.getByRole('button', { name: '上电' })).toBeTruthy();
  });
});

describe('声音组', () => {
  it('有静音按钮、音量滑块与数值；不显示速度', async () => {
    await setup();
    expect(screen.getByRole('heading', { name: '声音' })).toBeTruthy();
    expect(screen.getByRole('button', { name: '静音' })).toBeTruthy();
    expect(screen.getByRole('slider', { name: '音量' })).toBeTruthy();
    expect(screen.getByText('100%')).toBeTruthy();
    expect(screen.queryByText('速度')).toBeNull();
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
