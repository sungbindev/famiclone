/*
 * ------------------------------------------------------------------------
 * 名称：release-notes-prompt.test.ts
 * 说明：升级后提示更新记录的测试：版本变化时打开更新记录页并记下版本、版本没变不提示、全新安装（Squirrel 首次启动）只记录不提示、没有记录又不是全新安装时提示、不支持升级的运行方式什么都不做、打不开页面时交给错误回调。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的升级服务、界面状态与帮助文档窗口。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { UpdateStatus } from '../../contracts/system/generated/system';
import { showReleaseNotesOnce, type ReleaseNotesPromptOptions } from './release-notes-prompt';

function setup(overrides: { recorded?: string | null; state?: UpdateStatus['state']; firstRun?: boolean; open?: () => Promise<void> }) {
  const state = { get: vi.fn(() => ({ releaseNotesVersion: overrides.recorded ?? null })), updateReleaseNotesVersion: vi.fn() };
  const helpWindow = { open: vi.fn(overrides.open ?? (() => Promise.resolve())) };
  const onError = vi.fn();
  const options = {
    updater: { status: () => ({ state: overrides.state ?? 'idle' }) },
    currentVersion: '0.2.0',
    firstRun: overrides.firstRun ?? false,
    state,
    helpWindow,
    onError
  } as unknown as ReleaseNotesPromptOptions;
  return { run: () => showReleaseNotesOnce(options), state, helpWindow, onError };
}

describe('showReleaseNotesOnce', () => {
  it('记录的版本与当前版本不同：打开更新记录页，并记下当前版本', () => {
    const { run, state, helpWindow } = setup({ recorded: '0.1.0' });
    run();
    expect(helpWindow.open).toHaveBeenCalledWith('changelog');
    expect(state.updateReleaseNotesVersion).toHaveBeenCalledWith('0.2.0');
  });

  it('记录的版本就是当前版本：不提示，也不再写记录', () => {
    const { run, state, helpWindow } = setup({ recorded: '0.2.0' });
    run();
    expect(helpWindow.open).not.toHaveBeenCalled();
    expect(state.updateReleaseNotesVersion).not.toHaveBeenCalled();
  });

  it('全新安装（Squirrel 首次启动）：只记下当前版本，不提示', () => {
    const { run, state, helpWindow } = setup({ firstRun: true });
    run();
    expect(state.updateReleaseNotesVersion).toHaveBeenCalledWith('0.2.0');
    expect(helpWindow.open).not.toHaveBeenCalled();
  });

  it('没有记录又不是全新安装（从不会记录的旧版本升上来）：提示并记下当前版本', () => {
    const { run, state, helpWindow } = setup({ recorded: null });
    run();
    expect(helpWindow.open).toHaveBeenCalledWith('changelog');
    expect(state.updateReleaseNotesVersion).toHaveBeenCalledWith('0.2.0');
  });

  it('不支持升级的运行方式（开发版、免安装目录）：什么都不做', () => {
    const { run, state, helpWindow } = setup({ state: 'unsupported' });
    run();
    expect(helpWindow.open).not.toHaveBeenCalled();
    expect(state.updateReleaseNotesVersion).not.toHaveBeenCalled();
  });

  it('打不开页面时交给错误回调，版本已经记下', async () => {
    const failure = new Error('找不到帮助文档');
    const { run, state, onError } = setup({ recorded: '0.1.0', open: () => Promise.reject(failure) });
    run();
    await vi.waitFor(() => expect(onError).toHaveBeenCalledWith(failure));
    expect(state.updateReleaseNotesVersion).toHaveBeenCalledWith('0.2.0');
  });
});
