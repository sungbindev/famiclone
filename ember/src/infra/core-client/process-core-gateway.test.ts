/*
 * ------------------------------------------------------------------------
 * 名称：process-core-gateway.test.ts
 * 说明：核心网关的测试：用假的核心进程检查启动、请求与响应、取消、事件、异常退出后重启与放弃、启动失败与停止。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreHostInbound, CoreHostOutbound } from '../../contracts/core-gateway/core-messages';
import type { ErrorLogEntry } from '../../contracts/logging/error-logger';
import type { CoreProcess } from '../../platform/core-process/core-process';
import { ProcessCoreGateway } from './process-core-gateway';

/** 假的核心进程。 */
class FakeProcess implements CoreProcess {
  readonly sent: CoreHostInbound[] = [];
  killed = false;
  private messageListener: ((message: unknown) => void) | undefined;
  private exitListener: ((code: number) => void) | undefined;

  postMessage(message: unknown): void {
    this.sent.push(message as CoreHostInbound);
  }
  onMessage(listener: (message: unknown) => void): void {
    this.messageListener = listener;
  }
  onExit(listener: (code: number) => void): void {
    this.exitListener = listener;
  }
  kill(): void {
    this.killed = true;
  }
  reply(message: CoreHostOutbound): void {
    this.messageListener?.(message);
  }
  exit(code: number): void {
    this.exitListener?.(code);
  }
}

function setup(now: () => number = () => 0) {
  const processes: FakeProcess[] = [];
  const logs: ErrorLogEntry[] = [];
  let next = 0;
  const gateway = new ProcessCoreGateway({
    startProcess: () => {
      const child = new FakeProcess();
      processes.push(child);
      return child;
    },
    nativePath: 'core.node',
    userDataPath: 'data',
    appVersion: '0.1.0',
    logger: { write: (entry) => logs.push(entry) },
    now,
    createId: () => `id-${++next}`
  });
  gateway.start();
  return { gateway, processes, logs, current: () => processes[processes.length - 1]! };
}

describe('ProcessCoreGateway', () => {
  it('启动核心进程并在就绪后更新状态', () => {
    const { gateway, current } = setup();
    expect(current().sent[0]).toEqual({ type: 'start', nativePath: 'core.node', userDataPath: 'data', appVersion: '0.1.0' });
    expect(gateway.status()).toEqual({ state: 'starting' });
    current().reply({ type: 'ready', coreVersion: '1.0.0' });
    expect(gateway.status()).toEqual({ state: 'ready', coreVersion: '1.0.0' });
  });

  it('按请求标识返回结果与错误', async () => {
    const { gateway, current } = setup();
    const ok = gateway.request('system.version', {});
    const failed = gateway.request('system.version', {});
    expect(current().sent[1]).toEqual({ type: 'request', id: 'id-1', name: 'system.version', params: {} });
    current().reply({ type: 'response', id: 'id-2', ok: false, error: { code: 'core.internal', message: '坏了' } });
    current().reply({ type: 'response', id: 'id-1', ok: true, result: { appVersion: '0.1.0', coreVersion: '1.0.0' } });
    await expect(ok).resolves.toEqual({ appVersion: '0.1.0', coreVersion: '1.0.0' });
    await expect(failed).rejects.toMatchObject({ code: 'core.internal', message: '坏了' });
  });

  it('中止时发送取消并以 core.cancelled 拒绝', async () => {
    const { gateway, current } = setup();
    const controller = new AbortController();
    const pending = gateway.request('system.version', {}, { signal: controller.signal });
    controller.abort();
    await expect(pending).rejects.toMatchObject({ code: 'core.cancelled' });
    expect(current().sent).toContainEqual({ type: 'cancel', id: 'id-1' });
  });

  it('转发核心事件', () => {
    const { gateway, current } = setup();
    const listener = vi.fn();
    const off = gateway.onEvent(listener);
    current().reply({ type: 'event', name: 'demo.changed', data: { a: 1 } });
    off();
    current().reply({ type: 'event', name: 'demo.changed', data: { a: 2 } });
    expect(listener).toHaveBeenCalledTimes(1);
    expect(listener).toHaveBeenCalledWith('demo.changed', { a: 1 });
  });

  it('异常退出时拒绝进行中的请求、记录错误并重启', async () => {
    const { gateway, processes, logs, current } = setup();
    const pending = gateway.request('system.version', {});
    current().exit(1);
    await expect(pending).rejects.toMatchObject({ code: 'core.unavailable' });
    expect(processes).toHaveLength(2);
    expect(logs[0]).toMatchObject({ process: 'main', code: 'core.unavailable' });
    expect(gateway.status()).toEqual({ state: 'starting' });
  });

  it('一分钟内重启超过 3 次后停止重启', async () => {
    const { gateway, processes, current } = setup();
    for (let index = 0; index < 4; index += 1) current().exit(1);
    expect(processes).toHaveLength(4);
    expect(gateway.status().state).toBe('unavailable');
    await expect(gateway.request('system.version', {})).rejects.toMatchObject({ code: 'core.unavailable' });
  });

  it('超过时间窗口后重新计数', () => {
    let time = 0;
    const { processes, current } = setup(() => time);
    for (let index = 0; index < 3; index += 1) current().exit(1);
    time = 61_000;
    current().exit(1);
    expect(processes).toHaveLength(5);
  });

  it('启动失败时记录错误、结束进程且不重启', async () => {
    const { gateway, processes, logs, current } = setup();
    current().reply({ type: 'startFailed', error: { code: 'core.internal', message: '数据库打不开' } });
    expect(current().killed).toBe(true);
    current().exit(1);
    expect(processes).toHaveLength(1);
    expect(logs).toHaveLength(1);
    expect(gateway.status()).toEqual({ state: 'unavailable', error: { code: 'core.internal', message: '数据库打不开' } });
    await expect(gateway.request('system.version', {})).rejects.toMatchObject({ code: 'core.internal' });
  });

  it('停止时请求核心停止并等待退出', async () => {
    const { gateway, processes, current } = setup();
    const stopped = gateway.stop();
    expect(current().sent).toContainEqual({ type: 'stop' });
    current().exit(0);
    await stopped;
    expect(processes).toHaveLength(1);
    expect(gateway.status().state).toBe('unavailable');
  });
});
