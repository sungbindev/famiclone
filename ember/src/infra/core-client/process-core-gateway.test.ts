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
import { ProcessCoreGateway, type ProcessCoreGatewayOptions } from './process-core-gateway';

/** 假的核心进程。 */
class FakeProcess implements CoreProcess {
  readonly sent: CoreHostInbound[] = [];
  readonly transfers: Array<readonly object[] | undefined> = [];
  killed = false;
  private messageListener: ((message: unknown) => void) | undefined;
  private exitListener: ((code: number) => void) | undefined;

  postMessage(message: unknown, transfer?: readonly object[]): void {
    this.sent.push(message as CoreHostInbound);
    this.transfers.push(transfer);
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

function setup(now: () => number = () => 0, extra: Partial<ProcessCoreGatewayOptions> = {}) {
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
    createId: () => `id-${++next}`,
    ...extra
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

  it('状态变化时通知订阅者：核心异常退出后重启依次为启动中、就绪；取消订阅后不再通知', () => {
    const { gateway, current } = setup();
    const listener = vi.fn();
    const off = gateway.onStatusChange(listener);
    current().reply({ type: 'ready', coreVersion: '1.0.0' });
    current().exit(1);
    current().reply({ type: 'ready', coreVersion: '1.0.0' });
    expect(listener.mock.calls.map(([status]) => (status as { state: string }).state)).toEqual(['ready', 'starting', 'ready']);
    off();
    current().exit(1);
    expect(listener).toHaveBeenCalledTimes(3);
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

  it('给了预装插件目录时随启动消息交给核心', () => {
    const { current } = setup(() => 0, { bundledPluginsPath: 'D:/build/ember/plugins' });
    expect(current().sent[0]).toEqual({ type: 'start', nativePath: 'core.node', userDataPath: 'data', appVersion: '0.1.0', bundledPluginsPath: 'D:/build/ember/plugins' });
  });

  it('给了调试端口与令牌时随启动消息交给核心，没有时不带这两项', () => {
    const { current } = setup(() => 0, { debugPort: 5688, debugToken: 'secret' });
    expect(current().sent[0]).toEqual({ type: 'start', nativePath: 'core.node', userDataPath: 'data', appVersion: '0.1.0', debugPort: 5688, debugToken: 'secret' });
    const plain = setup();
    expect(plain.current().sent[0]).toEqual({ type: 'start', nativePath: 'core.node', userDataPath: 'data', appVersion: '0.1.0' });
  });

  it('把画面与输入端口随 streams 消息转移给核心进程', () => {
    const { gateway, current } = setup();
    const frames = { name: 'frames' };
    const input = { name: 'input' };
    expect(gateway.attachStreams({ frames, input })).toBe(true);
    expect(current().sent[1]).toEqual({ type: 'streams' });
    expect(current().transfers[1]).toEqual([frames, input]);
    // 普通消息不带转移列表。
    expect(current().transfers[0]).toBeUndefined();
  });

  it('核心进程不在运行时不交端口：重启之间、启动失败、停止中', async () => {
    const ports = { frames: {}, input: {} };
    const gaveUp = setup();
    for (let index = 0; index < 4; index += 1) gaveUp.current().exit(1);
    expect(gaveUp.gateway.attachStreams(ports)).toBe(false);

    const failed = setup();
    failed.current().reply({ type: 'startFailed', error: { code: 'core.internal', message: '数据库打不开' } });
    expect(failed.gateway.attachStreams(ports)).toBe(false);

    const stopping = setup();
    const stopped = stopping.gateway.stop();
    expect(stopping.gateway.attachStreams(ports)).toBe(false);
    stopping.current().exit(0);
    await stopped;
    expect(stopping.current().sent.filter((message) => message.type === 'streams')).toHaveLength(0);
  });

  it('核心进程每次启动（含重启）都通知订阅者，通知时已经可以交端口', () => {
    const processes: FakeProcess[] = [];
    const gateway = new ProcessCoreGateway({
      startProcess: () => {
        const child = new FakeProcess();
        processes.push(child);
        return child;
      },
      nativePath: 'core.node',
      userDataPath: 'data',
      appVersion: '0.1.0',
      logger: { write: () => undefined },
      now: () => 0
    });
    const accepted: boolean[] = [];
    const off = gateway.onCoreStarted(() => accepted.push(gateway.attachStreams({ frames: {}, input: {} })));
    gateway.start();
    expect(accepted).toEqual([true]);
    processes[0]!.exit(1);
    expect(processes).toHaveLength(2);
    expect(accepted).toEqual([true, true]);
    expect(processes[1]!.sent.map((message) => message.type)).toEqual(['start', 'streams']);
    off();
    processes[1]!.exit(1);
    expect(accepted).toHaveLength(2);
  });

  it('启动通知的监听者抛出异常时写日志，不影响核心启动', () => {
    const logs: ErrorLogEntry[] = [];
    const processes: FakeProcess[] = [];
    const gateway = new ProcessCoreGateway({
      startProcess: () => {
        const child = new FakeProcess();
        processes.push(child);
        return child;
      },
      nativePath: 'core.node',
      userDataPath: 'data',
      appVersion: '0.1.0',
      logger: { write: (entry) => logs.push(entry) }
    });
    gateway.onCoreStarted(() => {
      throw new Error('监听者出错');
    });
    expect(() => gateway.start()).not.toThrow();
    expect(logs).toHaveLength(1);
    expect(logs[0]?.message).toContain('监听者出错');
  });
});
