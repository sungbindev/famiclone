/*
 * ------------------------------------------------------------------------
 * 名称：open-bios.test.ts
 * 说明：machines.openBios 的测试：弹出的文件选择只显示 .nes、选定的路径与机型交给核心的 machines.setBios、取消时返回明确的结果而不是错误、核心的错误码原样返回、参数不合格时在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的文件选择窗口与假的核心网关；经主进程请求入口调用，覆盖从请求名到核心的整条转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { MACHINES_SCHEMA } from '../../../contracts/machines/generated/machines';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenBiosHandler } from './open-bios';

const MACHINE = { id: 'sb2000', name: '小霸王 2000', needsBios: true, supported: true, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } };

function setup(pick: FileDialog['pickFile'], request: CoreGateway['request'] = vi.fn().mockResolvedValue({ machine: MACHINE })) {
  const gateway: CoreGateway = { request, onEvent: () => () => undefined, onStatusChange: () => () => undefined, status: () => ({ state: 'starting' }) };
  const schemas = [MACHINES_SCHEMA];
  const core = { gateway, modules: [MACHINES_SCHEMA], schemas };
  const handler = createOpenBiosHandler({ dialog: { pickFile: pick, pickDirectory: vi.fn(), pickSaveFile: vi.fn() }, core });
  return { request, call: (params: unknown = { machine: 'sb2000' }) => handleMainRequest({ handlers: { 'machines.openBios': handler }, modules: [MACHINES_SCHEMA], schemas }, 'machines.openBios', params) };
}

describe('machines.openBios', () => {
  it('文件选择只显示 .nes，选定的路径与机型交给核心的 machines.setBios', async () => {
    const pick = vi.fn().mockResolvedValue('D:/bios/SB2K_BIOS.NES');
    const { request, call } = setup(pick);
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: false, machine: MACHINE } });
    expect(pick).toHaveBeenCalledWith({ title: '添加 BIOS', filters: [{ name: 'NES 文件', extensions: ['nes'] }] });
    expect(request).toHaveBeenCalledTimes(1);
    expect(request).toHaveBeenCalledWith('machines.setBios', { machine: 'sb2000', path: 'D:/bios/SB2K_BIOS.NES' });
  });

  it('用户取消选择时返回 canceled，不是错误，也不调用核心', async () => {
    const { request, call } = setup(vi.fn().mockResolvedValue(undefined));
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(request).not.toHaveBeenCalled();
  });

  it('核心拒绝文件时返回核心的错误码与说明', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'machines.biosInvalid', message: '小霸王 2000 的 BIOS 的 PRG 必须是 512KB' }));
    const { call } = setup(vi.fn().mockResolvedValue('D:/bios/bad.nes'), failing);
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'machines.biosInvalid', message: '小霸王 2000 的 BIOS 的 PRG 必须是 512KB' } });
  });

  it('参数不合格时在入口被拒绝，不弹出文件选择窗口', async () => {
    const pick = vi.fn();
    const { request, call } = setup(pick);
    await expect(call({})).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    expect(pick).not.toHaveBeenCalled();
    expect(request).not.toHaveBeenCalled();
  });

  it('文件选择窗口本身出错时返回 core.internal', async () => {
    const { request, call } = setup(vi.fn().mockRejectedValue(new Error('窗口失败')));
    await expect(call()).resolves.toMatchObject({ ok: false, error: { code: 'core.internal' } });
    expect(request).not.toHaveBeenCalled();
  });
});
