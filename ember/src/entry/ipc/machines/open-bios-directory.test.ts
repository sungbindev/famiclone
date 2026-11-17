/*
 * ------------------------------------------------------------------------
 * 名称：open-bios-directory.test.ts
 * 说明：machines.openBiosDirectory 的测试：成功时打开 BIOS 目录并返回空结果、打不开时返回 machines.biosDirectoryUnavailable 与原因、带多余参数的请求在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的 BIOS 目录；经主进程请求入口调用，覆盖从请求名到处理函数的转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { MACHINES_SCHEMA } from '../../../contracts/machines/generated/machines';
import type { BiosDirectory } from '../../../contracts/shell/bios-directory';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenBiosDirectoryHandler } from './open-bios-directory';

function setup(open: BiosDirectory['open']) {
  const handler = createOpenBiosDirectoryHandler({ biosDirectory: { open } });
  const schemas = [MACHINES_SCHEMA];
  return (params: unknown = {}) => handleMainRequest({ handlers: { 'machines.openBiosDirectory': handler }, modules: schemas, schemas }, 'machines.openBiosDirectory', params);
}

describe('machines.openBiosDirectory', () => {
  it('打开 BIOS 目录并返回空结果', async () => {
    const open = vi.fn().mockResolvedValue(undefined);
    await expect(setup(open)()).resolves.toEqual({ ok: true, result: {} });
    expect(open).toHaveBeenCalledTimes(1);
  });

  it('打不开时返回错误码与原因', async () => {
    const call = setup(vi.fn().mockRejectedValue(new Error('没有权限')));
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'machines.biosDirectoryUnavailable', message: '无法打开 BIOS 目录：没有权限' } });
  });

  it('带多余的参数时在入口被拒绝，不打开目录', async () => {
    const open = vi.fn();
    const reply = await setup(open)({ path: 'C:/Windows' });
    expect(reply.ok).toBe(false);
    expect(open).not.toHaveBeenCalled();
  });
});
