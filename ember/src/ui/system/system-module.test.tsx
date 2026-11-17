/*
 * ------------------------------------------------------------------------
 * 名称：system-module.test.tsx
 * 说明：系统模块登记项的测试：“帮助”菜单有“帮助文档”“打开日志目录”“检查更新”“关于 Ember”、打开帮助文档与打开日志目录都经常驻组件交来的接口发出请求、失败时用通知给出说明、检查更新命令经升级控制器发出请求、关于命令打开对话框、对话框已登记。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：命令只拿到外壳接口（假的），预加载接口由常驻组件交给模块，常驻组件需要外壳的上下文。
 * ------------------------------------------------------------------------
 */

import { render, waitFor } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi, ShellCommandDefinition } from '../shell/shell-registry';
import { createSystemModule, SYSTEM_COMMANDS } from './system-module';

function setup(openLogDirectory: () => Promise<unknown>, openHelp: () => Promise<unknown> = () => Promise.resolve({})) {
  const module = createSystemModule([], '2026-11-16 12:00:00');
  const system = {
    openLogDirectory: vi.fn(openLogDirectory),
    openHelp: vi.fn(openHelp),
    updateStatus: vi.fn(() => Promise.resolve({ status: { state: 'idle' } })),
    checkUpdate: vi.fn(() => Promise.resolve({ status: { state: 'checking' } })),
    installUpdate: vi.fn(() => Promise.resolve({}))
  };
  const bridge = { main: { system }, onUpdateStatusChange: vi.fn(() => () => undefined) } as unknown as EmberBridge;
  const shell = { notify: vi.fn(), openDialog: vi.fn(), refreshCommands: vi.fn() } as unknown as ShellApi;
  const Host = module.Host as NonNullable<typeof module.Host>;
  const view = render(
    <ShellApiContext.Provider value={shell}>
      <Host bridge={bridge} />
    </ShellApiContext.Provider>
  );
  const command = (id: string): ShellCommandDefinition => module.commands?.find((item) => item.id === id) as ShellCommandDefinition;
  return { module, bridge, shell, command, view, system };
}

describe('系统模块的登记', () => {
  it('“帮助”菜单有“帮助文档”“打开日志目录”“检查更新”与“关于 Ember”，后两者前面有分隔线；对话框已登记', () => {
    const { module, command } = setup(() => Promise.resolve({}));
    expect(module.menuItems).toEqual([
      { menu: 'help', command: SYSTEM_COMMANDS.openHelp },
      { menu: 'help', command: SYSTEM_COMMANDS.openLogDirectory },
      { menu: 'help', command: SYSTEM_COMMANDS.checkUpdate, separatorBefore: true },
      { menu: 'help', command: SYSTEM_COMMANDS.openAbout, separatorBefore: true }
    ]);
    expect(command(SYSTEM_COMMANDS.openHelp).title).toBe('帮助文档');
    expect(command(SYSTEM_COMMANDS.openLogDirectory).title).toBe('打开日志目录');
    expect(command(SYSTEM_COMMANDS.openAbout).title).toBe('关于 Ember');
    expect(module.dialogs?.map((dialog) => dialog.id)).toEqual(['system.about']);
  });

  it('检查更新：名称随升级状态变化，执行时经常驻组件交来的接口发出检查请求', async () => {
    const { bridge, shell, command, system } = setup(() => Promise.resolve({}));
    const check = command(SYSTEM_COMMANDS.checkUpdate);
    expect(typeof check.title === 'function' ? check.title() : check.title).toBe('检查更新');
    expect(check.enabled?.({ bottomPanelOpen: false, activePanelId: null, fullScreen: false, alwaysOnTop: false })).toBe(true);
    await waitFor(() => expect(bridge.onUpdateStatusChange).toHaveBeenCalledTimes(1));
    check.run(shell);
    await waitFor(() => expect(system.checkUpdate).toHaveBeenCalledWith({}));
  });

  it('打开帮助文档：经预加载接口发出请求，成功时没有通知', async () => {
    const { bridge, shell, command } = setup(() => Promise.resolve({}));
    command(SYSTEM_COMMANDS.openHelp).run(shell);
    await waitFor(() => expect(bridge.main.system.openHelp).toHaveBeenCalledWith({}));
    expect(shell.notify).not.toHaveBeenCalled();
  });

  it('打开帮助文档失败时用通知给出说明，没有说明时用默认文字', async () => {
    const failing = setup(() => Promise.resolve({}), () => Promise.reject({ code: 'system.helpUnavailable', message: '无法打开帮助文档：找不到帮助文档' }));
    failing.command(SYSTEM_COMMANDS.openHelp).run(failing.shell);
    await waitFor(() => expect(failing.shell.notify).toHaveBeenCalledWith('无法打开帮助文档：找不到帮助文档', 'error'));
    const silent = setup(() => Promise.resolve({}), () => Promise.reject(new Error('')));
    silent.command(SYSTEM_COMMANDS.openHelp).run(silent.shell);
    await waitFor(() => expect(silent.shell.notify).toHaveBeenCalledWith('无法打开帮助文档', 'error'));
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
    command(SYSTEM_COMMANDS.openHelp).run(shell);
    expect(bridge.main.system.openHelp).not.toHaveBeenCalled();
  });

  it('“关于 Ember”命令打开对话框', () => {
    const { shell, command } = setup(() => Promise.resolve({}));
    command(SYSTEM_COMMANDS.openAbout).run(shell);
    expect(shell.openDialog).toHaveBeenCalledWith('system.about');
  });
});
