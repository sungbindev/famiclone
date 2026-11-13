/*
 * ------------------------------------------------------------------------
 * 名称：main-request-ipc.test.ts
 * 说明：主进程请求入口的测试：只接受已登记模块与处理函数的请求、按 Schema 校验参数、把处理函数的错误转为错误码、发送方检查。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import type { IpcMain } from 'electron';
import { describe, expect, it, vi } from 'vitest';
import { CARTRIDGES_SCHEMA } from '../../../contracts/cartridges/generated/cartridges';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { RUNTIME_SCHEMA } from '../../../contracts/runtime/generated/runtime';
import { SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import { SHELL_CHANNELS } from '../../../contracts/shell/shell-bridge';
import { handleMainRequest, registerMainRequestIpc } from './main-request-ipc';

const MODULES = [SYSTEM_SCHEMA, CARTRIDGES_SCHEMA];
const SCHEMAS = [SYSTEM_SCHEMA, RUNTIME_SCHEMA, CARTRIDGES_SCHEMA];

function options(handler = vi.fn().mockResolvedValue({ canceled: true })) {
  return { handlers: { 'cartridges.open': handler }, modules: MODULES, schemas: SCHEMAS };
}

describe('handleMainRequest', () => {
  it('校验通过后交给处理函数并返回结果', async () => {
    const handler = vi.fn().mockResolvedValue({ canceled: true });
    await expect(handleMainRequest(options(handler), 'cartridges.open', {})).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(handler).toHaveBeenCalledWith({});
  });

  it('拒绝没有登记的请求名、模块与非字符串', async () => {
    const handler = vi.fn();
    const base = options(handler);
    for (const name of ['cartridges.missing', 'nothing.open', 'open', '.open', '', 42, null, undefined, {}]) {
      await expect(handleMainRequest(base, name, {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    }
    expect(handler).not.toHaveBeenCalled();
  });

  it('核心请求不能经主进程请求入口调用', async () => {
    const handler = vi.fn();
    // cartridges.load 是进入核心的请求（mainOnly），不是 mainRequests，即使有同名的处理函数也不接受。
    const reply = await handleMainRequest({ handlers: { 'cartridges.load': handler }, modules: MODULES, schemas: SCHEMAS }, 'cartridges.load', { path: 'x.nes' });
    expect(reply).toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    expect(handler).not.toHaveBeenCalled();
  });

  it('模块没有登记时拒绝，处理函数没有登记时也拒绝', async () => {
    await expect(handleMainRequest({ ...options(), modules: [SYSTEM_SCHEMA] }, 'cartridges.open', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    await expect(handleMainRequest({ ...options(), handlers: {} }, 'cartridges.open', {})).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
  });

  it('参数不合格时不调用处理函数', async () => {
    const handler = vi.fn();
    await expect(handleMainRequest(options(handler), 'cartridges.open', { path: 'C:/secret.nes' })).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    await expect(handleMainRequest(options(handler), 'cartridges.open', 'x')).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    expect(handler).not.toHaveBeenCalled();
  });

  it('把 CoreRequestError 转为错误码与说明', async () => {
    const handler = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'cartridges.notInes', message: '不是 NES 文件' }));
    await expect(handleMainRequest(options(handler), 'cartridges.open', {})).resolves.toEqual({ ok: false, error: { code: 'cartridges.notInes', message: '不是 NES 文件' } });
  });

  it('其他异常转为 core.internal 并交给日志回调，说明里不含内部细节', async () => {
    const onError = vi.fn();
    const handler = vi.fn().mockRejectedValue(new Error('C:\\内部路径出错'));
    const reply = await handleMainRequest({ ...options(handler), onError }, 'cartridges.open', {});
    expect(reply).toMatchObject({ ok: false, error: { code: 'core.internal' } });
    expect(JSON.stringify(reply)).not.toContain('内部路径');
    expect(onError).toHaveBeenCalledWith(expect.any(Error), 'cartridges.open');
  });
});

/** 登记 IPC 处理，返回登记的通道与监听函数。 */
function register(trusted: boolean, handler = vi.fn().mockResolvedValue({ canceled: true })) {
  const handle = vi.fn();
  const ipc = { handle } as unknown as IpcMain;
  registerMainRequestIpc({ ...options(handler), ipc, isTrustedSender: () => trusted });
  const [channel, listener] = handle.mock.calls[0] as [string, (event: unknown, name: unknown, params: unknown) => Promise<unknown>];
  return { channel, listener, handler };
}

describe('registerMainRequestIpc', () => {
  it('在主进程请求通道上登记', () => {
    expect(register(true).channel).toBe(SHELL_CHANNELS.mainRequest);
  });

  it('可信的发送方得到结果', async () => {
    const { listener } = register(true);
    await expect(listener({}, 'cartridges.open', {})).resolves.toEqual({ ok: true, result: { canceled: true } });
  });

  it('不可信的发送方被拒绝，处理函数不被调用', async () => {
    const { listener, handler } = register(false);
    await expect(Promise.resolve(listener({}, 'cartridges.open', {}))).resolves.toMatchObject({ ok: false, error: { code: 'core.unknownRequest' } });
    expect(handler).not.toHaveBeenCalled();
  });
});
