/*
 * ------------------------------------------------------------------------
 * 名称：diagnostics-client.test.ts
 * 说明：诊断请求器的测试：按编号配对回复、超时拒绝、关闭时拒绝、没人等的回复忽略。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { afterEach, describe, expect, it, vi } from 'vitest';
import { DiagnosticsClient } from './diagnostics-client';
import type { WorkerDiagnostics } from './worker-messages';

/** 只关心编号的假诊断数据。 */
function diagnostics(received: number): WorkerDiagnostics {
  return { received } as unknown as WorkerDiagnostics;
}

afterEach(() => {
  vi.useRealTimers();
});

describe('DiagnosticsClient', () => {
  it('发出带编号的请求，回复按编号交给对应的等待者', async () => {
    const posted: Array<{ id: number; reset: boolean }> = [];
    const client = new DiagnosticsClient((request) => posted.push({ id: request.id, reset: request.reset }));
    const first = client.read();
    const second = client.read(true);
    expect(posted).toEqual([
      { id: 0, reset: false },
      { id: 1, reset: true }
    ]);
    client.handleReply(1, diagnostics(2));
    client.handleReply(0, diagnostics(1));
    expect((await first).received).toBe(1);
    expect((await second).received).toBe(2);
  });

  it('没有人等的回复被忽略，同一个回复不会处理两次', async () => {
    const client = new DiagnosticsClient(() => undefined);
    const pending = client.read();
    client.handleReply(99, diagnostics(9));
    client.handleReply(0, diagnostics(1));
    client.handleReply(0, diagnostics(2));
    expect((await pending).received).toBe(1);
  });

  it('超时后拒绝，迟到的回复不再生效', async () => {
    vi.useFakeTimers();
    const client = new DiagnosticsClient(() => undefined);
    const pending = client.read();
    const result = expect(pending).rejects.toThrow('没有及时回复');
    await vi.advanceTimersByTimeAsync(5001);
    await result;
    expect(() => client.handleReply(0, diagnostics(1))).not.toThrow();
  });

  it('关闭时拒绝全部等待中的请求', async () => {
    const client = new DiagnosticsClient(() => undefined);
    const first = client.read();
    const second = client.read();
    const results = Promise.all([expect(first).rejects.toThrow('已关闭'), expect(second).rejects.toThrow('已关闭')]);
    client.dispose();
    await results;
  });
});
