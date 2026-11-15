/*
 * ------------------------------------------------------------------------
 * 名称：MachinePicker.test.tsx
 * 说明：面板标题栏里机型选择的测试：下拉列表显示已选机型并列出五个机型、不可选的注明原因且选了不生效、选可用的机型只改已选机型不开机、机型旁没有按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：用假的核心接口与真实的控制器。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it } from 'vitest';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';
import { MachinePicker } from './MachinePicker';

/** 接上假核心并渲染机型选择。 */
async function setup(status = machineStatus('off', 'paused')) {
  const fake = new FakeRuntimeBackend();
  fake.status = status;
  const controller = new RuntimeController();
  controller.attach(fake.backend, { notify: () => undefined, refreshCommands: () => undefined });
  render(
    <UiProvider>
      <MachinePicker controller={controller} />
    </UiProvider>
  );
  await act(async () => settle());
  return { fake, controller };
}

describe('机型下拉列表', () => {
  it('显示已选机型（默认经典 FC），列出五个机型，不可选的注明原因', async () => {
    await setup();
    const trigger = screen.getByRole('combobox', { name: '机型' });
    expect(trigger.textContent).toBe('经典 FC');
    await userEvent.click(trigger);
    expect(screen.getAllByRole('option').map((option) => option.textContent)).toEqual([
      '经典 FC',
      '小霸王 97暂未支持',
      '小霸王 2000未添加 BIOS，请在设置里添加',
      '步步高 1.0暂未支持',
      '步步高 2.0暂未支持'
    ]);
  });

  it('选可用的机型只改已选机型并保存，不开机；选不可用的机型不生效', async () => {
    const { fake, controller } = await setup();
    fake.machines = fake.machines.map((machine) => (machine.id === 'sb2000' ? { ...machine, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } } : machine));
    controller.attach(fake.backend, { notify: () => undefined, refreshCommands: () => undefined });
    await act(async () => settle());
    await userEvent.click(screen.getByRole('combobox', { name: '机型' }));
    await userEvent.click(screen.getByRole('option', { name: /小霸王 97/ }));
    expect(controller.snapshot().selectedMachine).toBe('fc');
    await userEvent.click(screen.getByRole('combobox', { name: '机型' }));
    await userEvent.click(screen.getByRole('option', { name: /小霸王 2000/ }));
    await act(async () => settle());
    expect(controller.snapshot().selectedMachine).toBe('sb2000');
    expect(fake.layoutPatches).toEqual([{ selectedMachine: 'sb2000' }]);
    expect(screen.getByRole('combobox', { name: '机型' }).textContent).toBe('小霸王 2000');
    expect(fake.calls.filter((call) => call.startsWith('machines.boot') || call.startsWith('runtime.power') || call.startsWith('runtime.reset'))).toEqual([]);
  });
});

describe('机型旁没有按钮', () => {
  it('打开卡带只在“文件”菜单里，机型旁没有按钮', async () => {
    await setup();
    expect(screen.queryByRole('button')).toBeNull();
  });
});
