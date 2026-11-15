/*
 * ------------------------------------------------------------------------
 * 名称：open-package.test.ts
 * 说明：mappers.openPackage 的测试：文件选择显示插件包与所有文件、选定的路径交给核心的 mappers.install、取消时返回 canceled 而不是错误且不调用核心、核心的错误码原样返回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的文件选择窗口与假的核心网关；经主进程请求入口调用，覆盖从请求名到核心的整条转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { MAPPERS_SCHEMA } from '../../../contracts/mappers/generated/mappers';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenPackageHandler } from './open-package';

const PLUGIN = { id: 'nrom', name: 'NROM', version: '1.0.0', publisher: 'Ember', host: ['fc'], devices: [], match: [{ mapper: 0 }], source: 'user', enabled: true };

function setup(pick: FileDialog['pickFile'], request: CoreGateway['request'] = vi.fn().mockResolvedValue({ plugin: PLUGIN })) {
  const gateway: CoreGateway = { request, onEvent: () => () => undefined, onStatusChange: () => () => undefined, status: () => ({ state: 'starting' }) };
  const schemas = [MAPPERS_SCHEMA];
  const core = { gateway, modules: schemas, schemas };
  const handler = createOpenPackageHandler({ dialog: { pickFile: pick, pickDirectory: vi.fn(), pickSaveFile: vi.fn() }, core });
  return { request, call: (params: unknown = {}) => handleMainRequest({ handlers: { 'mappers.openPackage': handler }, modules: schemas, schemas }, 'mappers.openPackage', params) };
}

describe('mappers.openPackage', () => {
  it('文件选择显示插件包与所有文件，选定的路径交给核心的 mappers.install', async () => {
    const pick = vi.fn().mockResolvedValue('D:/plugins/nrom.emap');
    const { request, call } = setup(pick);
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: false, plugin: PLUGIN } });
    expect(pick).toHaveBeenCalledWith({
      title: '安装 mapper 插件',
      filters: [
        { name: 'mapper 插件包', extensions: ['emap'] },
        { name: '所有文件', extensions: ['*'] }
      ]
    });
    expect(request).toHaveBeenCalledTimes(1);
    expect(request).toHaveBeenCalledWith('mappers.install', { path: 'D:/plugins/nrom.emap' });
  });

  it('用户取消选择时返回 canceled，不是错误，也不调用核心', async () => {
    const { request, call } = setup(vi.fn().mockResolvedValue(undefined));
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(request).not.toHaveBeenCalled();
  });

  it('核心拒绝插件包时返回核心的错误码与说明', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'mappers.invalidPackage', message: '插件包缺少 manifest.json' }));
    const { call } = setup(vi.fn().mockResolvedValue('D:/plugins/bad.emap'), failing);
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'mappers.invalidPackage', message: '插件包缺少 manifest.json' } });
  });
});
