/*
 * ------------------------------------------------------------------------
 * 名称：open-floppy.test.ts
 * 说明：media.openFloppy 的测试：弹出的文件选择显示软盘镜像与所有文件、选定的路径与驱动器编号交给核心的 media.floppyInsert、取消时返回明确的结果而不是错误、核心的错误码原样返回、参数不合格时在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的文件选择窗口与假的核心网关；经主进程请求入口调用，覆盖从请求名到核心的整条转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { MEDIA_SCHEMA } from '../../../contracts/media/generated/media';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenFloppyHandler } from './open-floppy';

const STATUS = { drive: 0, inserted: true, fileName: 'sys.img', writeProtected: false, modified: false, size: 1474560, cylinder: 0, motor: false, faults: { ejected: false, noIndex: false, seekFail: false, unresponsive: false, badSectors: [] } };

function setup(pick: FileDialog['pickFile'], request: CoreGateway['request'] = vi.fn().mockResolvedValue({ status: STATUS })) {
  const gateway: CoreGateway = { request, onEvent: () => () => undefined, onStatusChange: () => () => undefined, status: () => ({ state: 'starting' }) };
  const schemas = [MEDIA_SCHEMA];
  const core = { gateway, modules: [MEDIA_SCHEMA], schemas };
  const handler = createOpenFloppyHandler({ dialog: { pickFile: pick, pickDirectory: vi.fn() }, core });
  return { request, call: (params: unknown = { drive: 0 }) => handleMainRequest({ handlers: { 'media.openFloppy': handler }, modules: [MEDIA_SCHEMA], schemas }, 'media.openFloppy', params) };
}

describe('media.openFloppy', () => {
  it('文件选择显示软盘镜像与所有文件，选定的路径与驱动器编号交给核心的 media.floppyInsert', async () => {
    const pick = vi.fn().mockResolvedValue('D:/disks/sys.img');
    const { request, call } = setup(pick);
    await expect(call({ drive: 0 })).resolves.toEqual({ ok: true, result: { canceled: false, status: STATUS } });
    expect(pick).toHaveBeenCalledWith({
      title: '载入软盘镜像',
      filters: [
        { name: '软盘镜像', extensions: ['img', 'ima', 'vfd', 'dsk', 'flp'] },
        { name: '所有文件', extensions: ['*'] }
      ]
    });
    expect(request).toHaveBeenCalledTimes(1);
    expect(request).toHaveBeenCalledWith('media.floppyInsert', { drive: 0, path: 'D:/disks/sys.img' });
  });

  it('用户取消选择时返回 canceled，不是错误，也不调用核心', async () => {
    const { request, call } = setup(vi.fn().mockResolvedValue(undefined));
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(request).not.toHaveBeenCalled();
  });

  it('核心拒绝文件时返回核心的错误码与说明', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'media.imageInvalid', message: '镜像文件是空的' }));
    const { call } = setup(vi.fn().mockResolvedValue('D:/disks/empty.img'), failing);
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'media.imageInvalid', message: '镜像文件是空的' } });
  });

  it('参数不合格时在入口被拒绝，不弹出文件选择窗口', async () => {
    const pick = vi.fn();
    const { request, call } = setup(pick);
    await expect(call({})).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    expect(pick).not.toHaveBeenCalled();
    expect(request).not.toHaveBeenCalled();
  });
});
