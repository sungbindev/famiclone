/*
 * ------------------------------------------------------------------------
 * 名称：update-requests.test.ts
 * 说明：升级请求的测试：updateStatus 返回当前状态、checkUpdate 开始检查并返回当前状态、installUpdate 在新版本下载好时重启安装并返回空结果、没有下载好时返回 system.updateNotReady、带多余参数的请求在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的升级服务；经主进程请求入口调用，覆盖从请求名到处理函数的转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { AppUpdater } from '../../../contracts/system/app-updater';
import { SYSTEM_SCHEMA, type UpdateStatus } from '../../../contracts/system/generated/system';
import { handleMainRequest } from '../core/main-request-ipc';
import { createCheckUpdateHandler, createInstallUpdateHandler, createUpdateStatusHandler } from './update-requests';

function setup(status: UpdateStatus, installed = true) {
  const updater = { status: vi.fn(() => status), check: vi.fn(() => ({ state: 'checking' }) satisfies UpdateStatus), install: vi.fn(() => installed) } satisfies Pick<AppUpdater, 'status' | 'check' | 'install'>;
  const schemas = [SYSTEM_SCHEMA];
  const handlers = {
    'system.updateStatus': createUpdateStatusHandler({ updater }),
    'system.checkUpdate': createCheckUpdateHandler({ updater }),
    'system.installUpdate': createInstallUpdateHandler({ updater })
  };
  return { updater, call: (name: string, params: unknown = {}) => handleMainRequest({ handlers, modules: schemas, schemas }, name, params) };
}

describe('system.updateStatus', () => {
  it('返回当前状态', async () => {
    const { call } = setup({ state: 'ready', version: '0.2.0' });
    await expect(call('system.updateStatus')).resolves.toEqual({ ok: true, result: { status: { state: 'ready', version: '0.2.0' } } });
  });
});

describe('system.checkUpdate', () => {
  it('开始检查并返回升级服务给出的状态', async () => {
    const { call, updater } = setup({ state: 'idle' });
    await expect(call('system.checkUpdate')).resolves.toEqual({ ok: true, result: { status: { state: 'checking' } } });
    expect(updater.check).toHaveBeenCalledTimes(1);
  });

  it('带多余的参数时在入口被拒绝，不检查', async () => {
    const { call, updater } = setup({ state: 'idle' });
    const reply = await call('system.checkUpdate', { url: 'https://evil.example.com' });
    expect(reply.ok).toBe(false);
    expect(updater.check).not.toHaveBeenCalled();
  });
});

describe('system.installUpdate', () => {
  it('新版本下载好时重启安装并返回空结果', async () => {
    const { call, updater } = setup({ state: 'ready' });
    await expect(call('system.installUpdate')).resolves.toEqual({ ok: true, result: {} });
    expect(updater.install).toHaveBeenCalledTimes(1);
  });

  it('没有下载好的新版本时返回 system.updateNotReady', async () => {
    const { call } = setup({ state: 'idle' }, false);
    await expect(call('system.installUpdate')).resolves.toEqual({ ok: false, error: { code: 'system.updateNotReady', message: '还没有下载好的新版本' } });
  });
});
