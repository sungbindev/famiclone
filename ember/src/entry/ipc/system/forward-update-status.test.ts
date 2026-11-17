/*
 * ------------------------------------------------------------------------
 * 名称：forward-update-status.test.ts
 * 说明：升级状态推送的测试：状态变化时用升级状态通道发给页面、页面销毁后不再发送、取消转发后不再发送。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的页面与假的升级服务。
 * ------------------------------------------------------------------------
 */

import type { WebContents } from 'electron';
import { describe, expect, it, vi } from 'vitest';
import { SHELL_CHANNELS } from '../../../contracts/shell/shell-bridge';
import type { UpdateStatus } from '../../../contracts/system/generated/system';
import { forwardUpdateStatus } from './forward-update-status';

function setup() {
  let listener: ((status: UpdateStatus) => void) | undefined;
  const stop = vi.fn();
  const updater = {
    onStatusChange: (next: (status: UpdateStatus) => void) => {
      listener = next;
      return stop;
    }
  };
  const contents = { send: vi.fn(), isDestroyed: vi.fn(() => false) };
  const detach = forwardUpdateStatus(updater, contents as unknown as WebContents);
  return { contents, stop, detach, emit: (status: UpdateStatus) => listener?.(status) };
}

describe('forwardUpdateStatus', () => {
  it('状态变化时用升级状态通道发给页面', () => {
    const { contents, emit } = setup();
    emit({ state: 'ready', version: '0.2.0' });
    expect(contents.send).toHaveBeenCalledWith(SHELL_CHANNELS.updateStatusChanged, { state: 'ready', version: '0.2.0' });
  });

  it('页面销毁后不再发送', () => {
    const { contents, emit } = setup();
    contents.isDestroyed.mockReturnValue(true);
    emit({ state: 'checking' });
    expect(contents.send).not.toHaveBeenCalled();
  });

  it('返回的函数取消订阅', () => {
    const { stop, detach } = setup();
    detach();
    expect(stop).toHaveBeenCalledTimes(1);
  });
});
