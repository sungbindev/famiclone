/*
 * ------------------------------------------------------------------------
 * 名称：pick-directory.test.ts
 * 说明：preferences.pickDirectory 的测试：返回用户选定的文件夹、取消时返回 canceled 而不是错误、选择窗口本身出错时返回 core.internal、参数不合格时在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的文件选择窗口；经主进程请求入口调用，覆盖从请求名到处理函数的整条转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { PREFERENCES_SCHEMA } from '../../../contracts/preferences/generated/preferences';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { handleMainRequest } from '../core/main-request-ipc';
import { createPickDirectoryHandler } from './pick-directory';

function setup(pick: FileDialog['pickDirectory']) {
  const handler = createPickDirectoryHandler({ dialog: { pickFile: vi.fn(), pickDirectory: pick, pickSaveFile: vi.fn() } });
  const schemas = [PREFERENCES_SCHEMA];
  return { call: (params: unknown = {}) => handleMainRequest({ handlers: { 'preferences.pickDirectory': handler }, modules: [PREFERENCES_SCHEMA], schemas }, 'preferences.pickDirectory', params) };
}

describe('preferences.pickDirectory', () => {
  it('返回用户选定的文件夹', async () => {
    const pick = vi.fn().mockResolvedValue('D:/prints');
    const { call } = setup(pick);
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: false, path: 'D:/prints' } });
    expect(pick).toHaveBeenCalledWith({ title: '选择文件夹', purpose: 'folder' });
  });

  it('用户取消选择时返回 canceled，不是错误', async () => {
    const { call } = setup(vi.fn().mockResolvedValue(undefined));
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: true } });
  });

  it('选择窗口本身出错时返回 core.internal', async () => {
    const { call } = setup(vi.fn().mockRejectedValue(new Error('窗口失败')));
    await expect(call()).resolves.toMatchObject({ ok: false, error: { code: 'core.internal' } });
  });

  it('参数不合格时在入口被拒绝，不弹出选择窗口', async () => {
    const pick = vi.fn();
    const { call } = setup(pick);
    await expect(call('不是对象')).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    expect(pick).not.toHaveBeenCalled();
  });
});
