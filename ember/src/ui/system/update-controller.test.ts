/*
 * ------------------------------------------------------------------------
 * 名称：update-controller.test.ts
 * 说明：升级控制器的测试：读取初始状态时不通知、各状态下菜单项的名称与可用状态、手动检查的结果（正在检查、已是最新版本、发现新版本、失败、不支持）才通知、自动检查只在新版本下载好时通知、已下载好时执行菜单项会重启安装、请求失败的说明、断开后不再响应推送。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：预加载接口与外壳接口都是假的，状态推送由测试手动触发。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { UpdateStatus } from '../../contracts/system/generated/system';
import type { ShellApi } from '../shell/shell-registry';
import { UpdateController, type UpdateBridge } from './update-controller';

/** 让已排队的 Promise 回调都跑完。 */
async function settle(): Promise<void> {
  await new Promise((resolve) => setTimeout(resolve, 0));
}

function setup(initial: UpdateStatus = { state: 'idle' }, checkResult: UpdateStatus = { state: 'checking' }) {
  let push: ((status: UpdateStatus) => void) | undefined;
  const stop = vi.fn();
  const system = {
    updateStatus: vi.fn(() => Promise.resolve({ status: initial })),
    checkUpdate: vi.fn(() => Promise.resolve({ status: checkResult })),
    installUpdate: vi.fn(() => Promise.resolve({}))
  };
  const bridge = {
    main: { system },
    onUpdateStatusChange: (listener: (status: UpdateStatus) => void) => {
      push = listener;
      return stop;
    }
  } as unknown as UpdateBridge;
  const host = { notify: vi.fn(), refresh: vi.fn() };
  const shell = { notify: vi.fn() } as unknown as ShellApi;
  const controller = new UpdateController();
  const detach = controller.attach(bridge, host);
  return { controller, system, host, shell, stop, detach, push: (status: UpdateStatus) => push?.(status) };
}

describe('UpdateController', () => {
  it('读取初始状态时不通知，但让外壳刷新菜单', async () => {
    const { controller, host } = setup({ state: 'ready', version: '0.2.0' });
    await settle();
    expect(controller.title()).toBe('重启并安装更新（0.2.0）');
    expect(host.refresh).toHaveBeenCalledTimes(1);
    expect(host.notify).not.toHaveBeenCalled();
  });

  it('各状态下菜单项的名称与可用状态', () => {
    const { controller, push } = setup();
    expect([controller.title(), controller.canRun()]).toEqual(['检查更新', true]);
    push({ state: 'checking' });
    expect([controller.title(), controller.canRun()]).toEqual(['正在检查更新…', false]);
    push({ state: 'downloading' });
    expect([controller.title(), controller.canRun()]).toEqual(['正在下载更新…', false]);
    push({ state: 'ready' });
    expect([controller.title(), controller.canRun()]).toEqual(['重启并安装更新', true]);
    push({ state: 'error', message: '网络不通' });
    expect([controller.title(), controller.canRun()]).toEqual(['检查更新', true]);
    push({ state: 'unsupported' });
    expect([controller.title(), controller.canRun()]).toEqual(['检查更新', true]);
  });

  it('手动检查：提示正在检查，已是最新版本时通知', async () => {
    const { controller, system, shell, host, push } = setup();
    controller.run(shell);
    await settle();
    expect(system.checkUpdate).toHaveBeenCalledWith({});
    expect(shell.notify).toHaveBeenCalledWith('正在检查更新…', 'info');
    push({ state: 'checking' });
    push({ state: 'upToDate' });
    expect(host.notify).toHaveBeenCalledWith('已是最新版本。', 'info');
  });

  it('手动检查：发现新版本与下载好都通知，下载好的提示里有版本号与操作', async () => {
    const { controller, shell, host, push } = setup();
    controller.run(shell);
    await settle();
    push({ state: 'downloading' });
    push({ state: 'ready', version: '0.2.0' });
    expect(host.notify.mock.calls).toEqual([
      ['发现新版本，正在下载。', 'info'],
      ['新版本 0.2.0 已下载好，在“帮助”菜单里选择“重启并安装更新”即可使用。', 'info']
    ]);
  });

  it('手动检查失败时通知原因；自动检查失败不通知', async () => {
    const manual = setup();
    manual.controller.run(manual.shell);
    await settle();
    manual.push({ state: 'error', message: '网络不通' });
    expect(manual.host.notify).toHaveBeenCalledWith('检查更新失败：网络不通', 'error');

    const auto = setup();
    auto.push({ state: 'checking' });
    auto.push({ state: 'error', message: '网络不通' });
    expect(auto.host.notify).not.toHaveBeenCalled();
  });

  it('自动检查：只在新版本下载好时通知，已是最新版本与下载中都不打扰', () => {
    const { host, push } = setup();
    push({ state: 'checking' });
    push({ state: 'upToDate' });
    push({ state: 'checking' });
    push({ state: 'downloading' });
    expect(host.notify).not.toHaveBeenCalled();
    push({ state: 'ready' });
    expect(host.notify).toHaveBeenCalledWith('新版本已下载好，在“帮助”菜单里选择“重启并安装更新”即可使用。', 'info');
  });

  it('不支持升级时手动检查给出说明，之后自动检查的结果不再被当作手动', async () => {
    const { controller, shell, host, push } = setup({ state: 'unsupported' }, { state: 'unsupported' });
    controller.run(shell);
    await settle();
    expect(shell.notify).toHaveBeenCalledWith('当前运行方式不支持检查更新，请使用安装版。', 'info');
    push({ state: 'upToDate' });
    expect(host.notify).not.toHaveBeenCalled();
  });

  it('检查请求失败时用通知给出说明，没有说明时用默认文字', async () => {
    const failing = setup();
    failing.system.checkUpdate.mockRejectedValue({ code: 'core.internal', message: '请求失败' });
    failing.controller.run(failing.shell);
    await settle();
    expect(failing.shell.notify).toHaveBeenCalledWith('请求失败', 'error');

    const silent = setup();
    silent.system.checkUpdate.mockRejectedValue(new Error(''));
    silent.controller.run(silent.shell);
    await settle();
    expect(silent.shell.notify).toHaveBeenCalledWith('检查更新失败', 'error');
  });

  it('新版本下载好时执行菜单项会重启安装，失败时用通知给出说明', async () => {
    const { controller, system, shell, push } = setup();
    push({ state: 'ready', version: '0.2.0' });
    controller.run(shell);
    expect(system.installUpdate).toHaveBeenCalledWith({});
    expect(system.checkUpdate).not.toHaveBeenCalled();

    system.installUpdate.mockRejectedValue({ code: 'system.updateNotReady', message: '还没有下载好的新版本' });
    controller.run(shell);
    await settle();
    expect(shell.notify).toHaveBeenCalledWith('还没有下载好的新版本', 'error');
  });

  it('检查或下载过程中执行菜单项什么都不做', () => {
    const { controller, system, shell, push } = setup();
    push({ state: 'checking' });
    controller.run(shell);
    push({ state: 'downloading' });
    controller.run(shell);
    expect(system.checkUpdate).not.toHaveBeenCalled();
    expect(system.installUpdate).not.toHaveBeenCalled();
  });

  it('断开后取消订阅，不再响应推送，执行菜单项也什么都不做', () => {
    const { controller, system, shell, host, stop, detach, push } = setup();
    detach();
    expect(stop).toHaveBeenCalledTimes(1);
    push({ state: 'ready' });
    expect(host.notify).not.toHaveBeenCalled();
    controller.run(shell);
    expect(system.checkUpdate).not.toHaveBeenCalled();
  });
});
