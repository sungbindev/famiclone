/*
 * ------------------------------------------------------------------------
 * 名称：open-help.test.ts
 * 说明：system.openHelp 的测试：成功时打开帮助文档窗口并返回空结果、可以指定固定清单里的页面、清单之外的页面名在入口被拒绝、打不开时返回 system.helpUnavailable 与原因、带多余参数的请求在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：用假的帮助文档窗口；经主进程请求入口调用，覆盖从请求名到处理函数的转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import type { HelpWindow } from '../../../contracts/shell/help-window';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenHelpHandler } from './open-help';

function setup(open: HelpWindow['open']) {
  const handler = createOpenHelpHandler({ helpWindow: { open } });
  const schemas = [SYSTEM_SCHEMA];
  return (params: unknown = {}) => handleMainRequest({ handlers: { 'system.openHelp': handler }, modules: schemas, schemas }, 'system.openHelp', params);
}

describe('system.openHelp', () => {
  it('打开帮助文档窗口并返回空结果，没有指定页面', async () => {
    const open = vi.fn().mockResolvedValue(undefined);
    await expect(setup(open)()).resolves.toEqual({ ok: true, result: {} });
    expect(open).toHaveBeenCalledTimes(1);
    expect(open).toHaveBeenCalledWith(undefined);
  });

  it('指定页面时把页面名交给帮助文档窗口', async () => {
    const open = vi.fn().mockResolvedValue(undefined);
    await expect(setup(open)({ page: 'changelog' })).resolves.toEqual({ ok: true, result: {} });
    expect(open).toHaveBeenCalledWith('changelog');
  });

  it('页面名不在固定的清单里时在入口被拒绝，不打开窗口', async () => {
    const open = vi.fn();
    for (const page of ['../manual/faq', 'C:/Windows', 5]) expect((await setup(open)({ page })).ok).toBe(false);
    expect(open).not.toHaveBeenCalled();
  });

  it('打不开时返回错误码与原因', async () => {
    const call = setup(vi.fn().mockRejectedValue(new Error('找不到帮助文档')));
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'system.helpUnavailable', message: '无法打开帮助文档：找不到帮助文档' } });
  });

  it('带多余的参数时在入口被拒绝，不打开窗口', async () => {
    const open = vi.fn();
    const reply = await setup(open)({ path: 'C:/Windows' });
    expect(reply.ok).toBe(false);
    expect(open).not.toHaveBeenCalled();
  });
});
