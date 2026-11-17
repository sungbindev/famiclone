/*
 * ------------------------------------------------------------------------
 * 名称：UpdateTitleBarItem.test.tsx
 * 说明：标题栏“更新”按钮的测试：新版本没有下载好时不显示，下载好后显示、提示里带版本号，点击后请求重启并安装更新，请求失败时用外壳通知给出说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：预加载接口与外壳接口都是假的，状态推送由测试手动触发；组件不自己订阅状态，由外壳重新渲染，这里用 rerender 模拟。
 * ------------------------------------------------------------------------
 */

import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it, vi } from 'vitest';
import type { UpdateStatus } from '../../contracts/system/generated/system';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi } from '../shell/shell-registry';
import { UpdateController, type UpdateBridge } from './update-controller';
import { UpdateTitleBarItem } from './UpdateTitleBarItem';

async function setup(initial: UpdateStatus = { state: 'idle' }) {
  let push: ((status: UpdateStatus) => void) | undefined;
  const system = {
    updateStatus: vi.fn(() => Promise.resolve({ status: initial })),
    checkUpdate: vi.fn(() => Promise.resolve({ status: { state: 'checking' } })),
    installUpdate: vi.fn(() => Promise.resolve({}))
  };
  const bridge = {
    main: { system },
    onUpdateStatusChange: (listener: (status: UpdateStatus) => void) => {
      push = listener;
      return () => undefined;
    }
  } as unknown as UpdateBridge;
  const shell = { notify: vi.fn() } as unknown as ShellApi;
  const controller = new UpdateController();
  controller.attach(bridge, { notify: vi.fn(), refresh: vi.fn() });
  // 等初始状态读完，否则它会覆盖之后手动推送的状态。
  await new Promise((resolve) => setTimeout(resolve, 0));
  // 每次都创建新元素：同一个元素重新渲染时 React 会跳过。
  const tree = () => (
    <ShellApiContext.Provider value={shell}>
      <UpdateTitleBarItem controller={controller} />
    </ShellApiContext.Provider>
  );
  return { system, shell, tree, push: (status: UpdateStatus) => push?.(status) };
}

describe('UpdateTitleBarItem', () => {
  it('新版本没有下载好时不显示，下载好后显示“更新”按钮，提示里带版本号', async () => {
    const { tree, push } = await setup();
    const view = render(tree());
    expect(screen.queryByRole('button', { name: '更新' })).toBeNull();
    push({ state: 'checking' });
    push({ state: 'downloading' });
    view.rerender(tree());
    expect(screen.queryByRole('button', { name: '更新' })).toBeNull();
    push({ state: 'ready', version: '0.2.0' });
    view.rerender(tree());
    expect(screen.getByRole('button', { name: '更新' }).getAttribute('title')).toBe('重启并安装更新（0.2.0）');
  });

  it('点击后请求重启并安装更新', async () => {
    const { tree, push, system } = await setup();
    const view = render(tree());
    push({ state: 'ready', version: '0.2.0' });
    view.rerender(tree());
    await userEvent.click(screen.getByRole('button', { name: '更新' }));
    expect(system.installUpdate).toHaveBeenCalledWith({});
    expect(system.checkUpdate).not.toHaveBeenCalled();
  });

  it('重启安装失败时用外壳通知给出说明', async () => {
    const { tree, push, system, shell } = await setup();
    system.installUpdate.mockRejectedValue({ code: 'system.updateNotReady', message: '还没有下载好的新版本' });
    const view = render(tree());
    push({ state: 'ready' });
    view.rerender(tree());
    await userEvent.click(screen.getByRole('button', { name: '更新' }));
    await waitFor(() => expect(shell.notify).toHaveBeenCalledWith('还没有下载好的新版本', 'error'));
  });
});
