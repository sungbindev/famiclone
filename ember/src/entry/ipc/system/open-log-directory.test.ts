/*
 * ------------------------------------------------------------------------
 * 名称：open-log-directory.test.ts
 * 说明：system.openLogDirectory 的测试：成功时打开日志目录并返回空结果、打不开时返回 system.logDirectoryUnavailable 与原因、带多余参数的请求在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的日志目录；经主进程请求入口调用，覆盖从请求名到处理函数的转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import type { LogDirectory } from '../../../contracts/shell/log-directory';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenLogDirectoryHandler } from './open-log-directory';

function setup(open: LogDirectory['open']) {
  const handler = createOpenLogDirectoryHandler({ logDirectory: { open } });
  const schemas = [SYSTEM_SCHEMA];
  return (params: unknown = {}) => handleMainRequest({ handlers: { 'system.openLogDirectory': handler }, modules: schemas, schemas }, 'system.openLogDirectory', params);
}

describe('system.openLogDirectory', () => {
  it('打开日志目录并返回空结果', async () => {
    const open = vi.fn().mockResolvedValue(undefined);
    await expect(setup(open)()).resolves.toEqual({ ok: true, result: {} });
    expect(open).toHaveBeenCalledTimes(1);
  });

  it('打不开时返回错误码与原因', async () => {
    const call = setup(vi.fn().mockRejectedValue(new Error('没有权限')));
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'system.logDirectoryUnavailable', message: '无法打开日志目录：没有权限' } });
  });

  it('带多余的参数时在入口被拒绝，不打开目录', async () => {
    const open = vi.fn();
    const reply = await setup(open)({ path: 'C:/Windows' });
    expect(reply.ok).toBe(false);
    expect(open).not.toHaveBeenCalled();
  });
});
