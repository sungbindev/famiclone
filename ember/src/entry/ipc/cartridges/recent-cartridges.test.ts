/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges.test.ts
 * 说明：cartridges.recent 与 cartridges.openRecent 的测试：清单只给显示名与编号、按编号重新打开（路径来自主进程保存的清单）、编号无效时返回 cartridges.recentNotFound、加载失败时移出清单、参数不合格时在入口被拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：经主进程请求入口调用，覆盖从请求名到核心的整条转接；清单用假的内存服务。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { CARTRIDGES_SCHEMA } from '../../../contracts/cartridges/generated/cartridges';
import type { RecentCartridgesService } from '../../../contracts/cartridges/recent-cartridges-service';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { RUNTIME_SCHEMA } from '../../../contracts/runtime/generated/runtime';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenRecentCartridgeHandler, createRecentCartridgesHandler } from './recent-cartridges';

const LOADED = { cartridge: { path: 'D:/roms/a.nes' }, state: { power: 'on', mode: 'running' } };

function setup(request: CoreGateway['request'] = vi.fn().mockResolvedValue(LOADED)) {
  const paths = new Map<number, string>([
    [1, 'D:/roms/a.nes'],
    [2, 'D:/roms/b.nes']
  ]);
  const calls: string[] = [];
  const recent: RecentCartridgesService = {
    list: () => [...paths].map(([id, path]) => ({ id, name: path.split('/').pop() ?? path })),
    add: (path) => calls.push(`add ${path}`),
    remove: (path) => calls.push(`remove ${path}`),
    resolve: (id) => paths.get(id),
    flush: () => undefined
  };
  const gateway: CoreGateway = { request, onEvent: () => () => undefined, onStatusChange: () => () => undefined, status: () => ({ state: 'starting' }) };
  const schemas = [RUNTIME_SCHEMA, CARTRIDGES_SCHEMA];
  const options = { core: { gateway, modules: [CARTRIDGES_SCHEMA], schemas }, recent };
  const handling = {
    handlers: { 'cartridges.recent': createRecentCartridgesHandler(options), 'cartridges.openRecent': createOpenRecentCartridgeHandler(options) },
    modules: [CARTRIDGES_SCHEMA],
    schemas
  };
  return { request, calls, call: (name: string, params: unknown) => handleMainRequest(handling, name, params) };
}

describe('cartridges.recent', () => {
  it('只给编号与显示名，不含路径', async () => {
    const { call } = setup();
    const reply = await call('cartridges.recent', {});
    expect(reply).toEqual({
      ok: true,
      result: {
        items: [
          { id: 1, name: 'a.nes' },
          { id: 2, name: 'b.nes' }
        ]
      }
    });
    expect(JSON.stringify(reply)).not.toContain('D:/roms');
  });
});

describe('cartridges.openRecent', () => {
  it('按编号取出路径交给核心的 cartridges.load，并记入清单', async () => {
    const { request, calls, call } = setup();
    await expect(call('cartridges.openRecent', { id: 2 })).resolves.toEqual({ ok: true, result: { loaded: LOADED } });
    expect(request).toHaveBeenCalledWith('cartridges.load', { path: 'D:/roms/b.nes' });
    expect(calls).toEqual(['add D:/roms/b.nes']);
  });

  it('编号不在清单里时返回 cartridges.recentNotFound，不调用核心', async () => {
    const { request, call } = setup();
    await expect(call('cartridges.openRecent', { id: 99 })).resolves.toMatchObject({ ok: false, error: { code: 'cartridges.recentNotFound' } });
    expect(request).not.toHaveBeenCalled();
  });

  it('加载失败时返回核心的错误码，并把卡带移出清单', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'cartridges.fileNotFound', message: '找不到文件' }));
    const { calls, call } = setup(failing);
    await expect(call('cartridges.openRecent', { id: 1 })).resolves.toEqual({ ok: false, error: { code: 'cartridges.fileNotFound', message: '找不到文件' } });
    expect(calls).toEqual(['add D:/roms/a.nes', 'remove D:/roms/a.nes']);
  });

  it('参数不合格（缺编号、编号不是正整数）在入口被拒绝', async () => {
    const { request, call } = setup();
    for (const params of [{}, { id: 0 }, { id: 1.5 }, { id: '1' }]) {
      await expect(call('cartridges.openRecent', params)).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    }
    expect(request).not.toHaveBeenCalled();
  });
});
