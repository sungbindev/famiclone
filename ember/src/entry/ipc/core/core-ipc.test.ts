/*
 * ------------------------------------------------------------------------
 * 名称：core-ipc.test.ts
 * 说明：核心请求入口的测试：只接受已登记模块的请求、按 Schema 校验参数、把网关的错误转为错误码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import { handleCoreRequest } from './core-ipc';

function gatewayWith(request: CoreGateway['request']): CoreGateway {
  return { request, onEvent: () => () => undefined, status: () => ({ state: 'starting' }) };
}

describe('handleCoreRequest', () => {
  it('校验通过后交给核心网关', async () => {
    const request = vi.fn().mockResolvedValue({ appVersion: '0.1.0', coreVersion: '1.0.0' });
    const reply = await handleCoreRequest({ gateway: gatewayWith(request), modules: [SYSTEM_SCHEMA], schemas: [SYSTEM_SCHEMA] }, 'system.version', {});
    expect(reply).toEqual({ ok: true, result: { appVersion: '0.1.0', coreVersion: '1.0.0' } });
    expect(request).toHaveBeenCalledWith('system.version', {});
  });

  it('拒绝未登记的模块与请求名', async () => {
    const request = vi.fn();
    const options = { gateway: gatewayWith(request), modules: [], schemas: [SYSTEM_SCHEMA] };
    await expect(handleCoreRequest(options, 'system.version', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    await expect(handleCoreRequest({ ...options, modules: [SYSTEM_SCHEMA] }, 'system.missing', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    await expect(handleCoreRequest({ ...options, modules: [SYSTEM_SCHEMA] }, 42, {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    expect(request).not.toHaveBeenCalled();
  });

  it('只能由主进程发起的请求不接受渲染进程发送', async () => {
    const schema = { ...SYSTEM_SCHEMA, requests: { version: { ...SYSTEM_SCHEMA.requests.version!, mainOnly: true } } };
    const request = vi.fn().mockResolvedValue({ appVersion: '0.1.0', coreVersion: '1.0.0' });
    const options = { gateway: gatewayWith(request), modules: [schema], schemas: [schema] };
    await expect(handleCoreRequest(options, 'system.version', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    expect(request).not.toHaveBeenCalled();
    await expect(handleCoreRequest({ ...options, allowMainOnly: true }, 'system.version', {})).resolves.toMatchObject({ ok: true });
  });

  it('只经调试服务开放的请求不接受界面与主进程发送', async () => {
    const schema = { ...SYSTEM_SCHEMA, requests: { version: { ...SYSTEM_SCHEMA.requests.version!, debugOnly: true } } };
    const request = vi.fn().mockResolvedValue({ appVersion: '0.1.0', coreVersion: '1.0.0' });
    const options = { gateway: gatewayWith(request), modules: [schema], schemas: [schema] };
    await expect(handleCoreRequest(options, 'system.version', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    await expect(handleCoreRequest({ ...options, allowMainOnly: true }, 'system.version', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    expect(request).not.toHaveBeenCalled();
  });

  it('参数不合格时不调用核心', async () => {
    const request = vi.fn();
    const reply = await handleCoreRequest({ gateway: gatewayWith(request), modules: [SYSTEM_SCHEMA], schemas: [SYSTEM_SCHEMA] }, 'system.version', { extra: 1 });
    expect(reply).toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    expect(request).not.toHaveBeenCalled();
  });

  it('把网关的错误转为错误码与说明', async () => {
    const options = { modules: [SYSTEM_SCHEMA], schemas: [SYSTEM_SCHEMA] };
    const coreError = gatewayWith(() => Promise.reject(new CoreRequestError({ code: 'core.unavailable', message: '核心不可用' })));
    await expect(handleCoreRequest({ ...options, gateway: coreError }, 'system.version', {})).resolves.toEqual({ ok: false, error: { code: 'core.unavailable', message: '核心不可用' } });
    const otherError = gatewayWith(() => Promise.reject(new Error('意外')));
    await expect(handleCoreRequest({ ...options, gateway: otherError }, 'system.version', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.internal' } });
  });
});
