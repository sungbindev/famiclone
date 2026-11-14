/*
 * ------------------------------------------------------------------------
 * 名称：open-cartridge.test.ts
 * 说明：cartridges.open 的测试：弹出的文件选择只显示 .nes、选定的路径交给核心的 cartridges.load、取消时返回明确的结果而不是错误、加载失败的错误码原样返回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的文件选择窗口与假的核心网关；经主进程请求入口调用，覆盖从请求名到核心的整条转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { CARTRIDGES_SCHEMA } from '../../../contracts/cartridges/generated/cartridges';
import type { RecentCartridgesService } from '../../../contracts/cartridges/recent-cartridges-service';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { RUNTIME_SCHEMA } from '../../../contracts/runtime/generated/runtime';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenCartridgeHandler } from './open-cartridge';

const LOADED = {
  cartridge: {
    path: 'D:/roms/idle.nes',
    mapper: 0,
    variant: 0,
    prgSize: 16384,
    chrSize: 0,
    mirroring: 'horizontal',
    battery: false,
    crc32Prg: '0A1B2C3D',
    crc32All: '0A1B2C3D',
    pluginId: 'nrom',
    pluginName: 'NROM',
    pluginVersion: '1.0.0',
    host: 'fc',
    storage: []
  },
  state: { power: 'on', mode: 'running' }
};

function setup(pick: FileDialog['pickFile'], request: CoreGateway['request'] = vi.fn().mockResolvedValue(LOADED)) {
  /** 按发生的先后记下“加入、请求核心、移出”，用来核对“先记入、失败再移出”的顺序。 */
  const calls: string[] = [];
  const recording: CoreGateway['request'] = (name, params) => {
    calls.push(`request ${name}`);
    return request(name, params);
  };
  const gateway: CoreGateway = { request: recording, onEvent: () => () => undefined, onStatusChange: () => () => undefined, status: () => ({ state: 'starting' }) };
  const schemas = [RUNTIME_SCHEMA, CARTRIDGES_SCHEMA];
  const core = { gateway, modules: [CARTRIDGES_SCHEMA], schemas };
  const recent: RecentCartridgesService = {
    list: () => [],
    add: (path) => calls.push(`add ${path}`),
    remove: (path) => calls.push(`remove ${path}`),
    resolve: () => undefined,
    flush: () => undefined
  };
  const handler = createOpenCartridgeHandler({ dialog: { pickFile: pick }, core, recent });
  return { gateway, request, calls, call: () => handleMainRequest({ handlers: { 'cartridges.open': handler }, modules: [CARTRIDGES_SCHEMA], schemas }, 'cartridges.open', {}) };
}

describe('cartridges.open', () => {
  it('文件选择只显示 .nes，选定的路径交给核心的 cartridges.load', async () => {
    const pick = vi.fn().mockResolvedValue('D:/roms/idle.nes');
    const { request, call } = setup(pick);
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: false, loaded: LOADED } });
    expect(pick).toHaveBeenCalledWith({ title: '打开卡带', filters: [{ name: 'NES 卡带', extensions: ['nes'] }] });
    expect(request).toHaveBeenCalledTimes(1);
    expect(request).toHaveBeenCalledWith('cartridges.load', { path: 'D:/roms/idle.nes' });
  });

  it('用户取消选择时返回 canceled，不是错误，也不调用核心，不记入最近打开', async () => {
    const { request, calls, call } = setup(vi.fn().mockResolvedValue(undefined));
    await expect(call()).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(request).not.toHaveBeenCalled();
    expect(calls).toEqual([]);
  });

  it('加载成功的卡带记入最近打开，记入在加载之前', async () => {
    const { calls, call } = setup(vi.fn().mockResolvedValue('D:/roms/idle.nes'));
    await call();
    expect(calls).toEqual(['add D:/roms/idle.nes', 'request cartridges.load']);
  });

  it('核心加载失败时返回核心的错误码与说明，并把卡带移出最近打开', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'cartridges.notInes', message: '文件不是 NES 卡带' }));
    const { calls, call } = setup(vi.fn().mockResolvedValue('D:/roms/bad.nes'), failing);
    await expect(call()).resolves.toEqual({ ok: false, error: { code: 'cartridges.notInes', message: '文件不是 NES 卡带' } });
    expect(calls).toEqual(['add D:/roms/bad.nes', 'request cartridges.load', 'remove D:/roms/bad.nes']);
  });

  it('文件选择窗口本身出错时返回 core.internal', async () => {
    const { request, call } = setup(vi.fn().mockRejectedValue(new Error('窗口失败')));
    await expect(call()).resolves.toMatchObject({ ok: false, error: { code: 'core.internal' } });
    expect(request).not.toHaveBeenCalled();
  });
});
