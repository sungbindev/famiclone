/*
 * ------------------------------------------------------------------------
 * 名称：system-module.test.tsx
 * 说明：系统模块登记项的测试：“帮助”菜单有“打开日志目录”“关于 Ember”、打开日志目录经常驻组件交来的接口发出请求、失败时用通知给出说明、关于命令打开对话框、对话框已登记。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：命令只拿到外壳接口（假的），预加载接口由常驻组件交给模块。
 * ------------------------------------------------------------------------
 */

import { render, waitFor } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellApi, ShellCommandDefinition } from '../shell/shell-registry';
import { createSystemModule, SYSTEM_COMMANDS } from './system-module';

function setup(openLogDirectory: () => Promise<unknown>) {
  const module = createSystemModule([]);
  const bridge = { main: { system: { openLogDirectory: vi.fn(openLogDirectory) } } } as unknown as EmberBridge;
  const shell = { notify: vi.fn(), openDialog: vi.fn() } as unknown as ShellApi;
  const Host = module.Host as NonNullable<typeof module.Host>;
  const view = render(<Host bridge={bridge} />);
  const command = (id: string): ShellCommandDefinition => module.commands?.find((item) => item.id === id) as ShellCommandDefinition;
  return { module, bridge, shell, command, view };
}

describe('系统模块的登记', () => {
  it('“帮助”菜单有“打开日志目录”与“关于 Ember”，后者前面有分隔线；对话框已登记', () => {
    const { module, command } = setup(() => Promise.resolve({}));
    expect(module.menuItems).toEqual([
      { menu: 'help', command: SYSTEM_COMMANDS.openLogDirectory },
      { menu: 'help', command: SYSTEM_COMMANDS.openAbout, separatorBefore: true }
    ]);
    expect(command(SYSTEM_COMMANDS.openLogDirectory).title).toBe('打开日志目录');
    expect(command(SYSTEM_COMMANDS.openAbout).title).toBe('关于 Ember');
    expect(module.dialogs?.map((dialog) => dialog.id)).toEqual(['system.about']);
  });

  it('打开日志目录：经预加载接口发出请求，成功时没有通知', async () => {
    const { bridge, shell, command } = setup(() => Promise.resolve({}));
    command(SYSTEM_COMMANDS.openLogDirectory).run(shell);
    await waitFor(() => expect(bridge.main.system.openLogDirectory).toHaveBeenCalledWith({}));
    expect(shell.notify).not.toHaveBeenCalled();
  });

  it('打开日志目录失败时用通知给出说明', async () => {
    const { shell, command } = setup(() => Promise.reject({ code: 'system.logDirectoryUnavailable', message: '无法打开日志目录：没有权限' }));
    command(SYSTEM_COMMANDS.openLogDirectory).run(shell);
    await waitFor(() => expect(shell.notify).toHaveBeenCalledWith('无法打开日志目录：没有权限', 'error'));
  });

  it('常驻组件卸载后没有接口，命令什么都不做', () => {
    const { bridge, shell, command, view } = setup(() => Promise.resolve({}));
    view.unmount();
    command(SYSTEM_COMMANDS.openLogDirectory).run(shell);
    expect(bridge.main.system.openLogDirectory).not.toHaveBeenCalled();
  });

  it('“关于 Ember”命令打开对话框', () => {
    const { shell, command } = setup(() => Promise.resolve({}));
    command(SYSTEM_COMMANDS.openAbout).run(shell);
    expect(shell.openDialog).toHaveBeenCalledWith('system.about');
  });
});
