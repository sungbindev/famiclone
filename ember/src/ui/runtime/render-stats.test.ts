/*
 * ------------------------------------------------------------------------
 * 名称：render-stats.test.ts
 * 说明：渲染端统计的测试：帧数、速率、帧号缺口、延迟摘要（平均、p95、最大）、统计重置。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { RenderStatistics, summarizeLatency } from './render-stats';

describe('summarizeLatency', () => {
  it('没有样本时全为 0', () => {
    expect(summarizeLatency([])).toEqual({ count: 0, average: 0, p95: 0, max: 0, min: 0 });
  });

  it('按最近秩法取 p95', () => {
    const samples = Array.from({ length: 100 }, (_, index) => index + 1);
    expect(summarizeLatency(samples)).toEqual({ count: 100, average: 50.5, p95: 95, max: 100, min: 1 });
  });

  it('单个样本的各项都等于它，输入顺序不影响结果', () => {
    expect(summarizeLatency([7])).toEqual({ count: 1, average: 7, p95: 7, max: 7, min: 7 });
    expect(summarizeLatency([3, 1, 2])).toMatchObject({ average: 2, max: 3, min: 1 });
  });
});

describe('RenderStatistics', () => {
  it('没有帧时全为 0', () => {
    const snapshot = new RenderStatistics().snapshot();
    expect(snapshot).toMatchObject({ received: 0, rendered: 0, dropped: 0, invalid: 0, coreMissed: 0, hostDropped: 0, coreFps: 0, receivedFps: 0, renderedFps: 0 });
  });

  it('按间隔计算收到、渲染与核心帧率', () => {
    const stats = new RenderStatistics();
    // 每 20 毫秒一帧（50 帧每秒），共 11 帧，10 个间隔。
    for (let index = 0; index < 11; index += 1) {
      stats.onReceived(index + 1, 1000 + index * 20, 0, 500 + index * 20, 1002 + index * 20);
      stats.onRendered(500 + index * 20, 505 + index * 20);
    }
    const snapshot = stats.snapshot();
    expect(snapshot.received).toBe(11);
    expect(snapshot.rendered).toBe(11);
    expect(snapshot.spanMs).toBe(200);
    expect(snapshot.receivedFps).toBeCloseTo(50, 6);
    expect(snapshot.renderedFps).toBeCloseTo(50, 6);
    expect(snapshot.coreFps).toBeCloseTo(50, 6);
    expect(snapshot.lastFrame).toBe(11);
  });

  it('统计两种延迟的平均、p95 与最大', () => {
    const stats = new RenderStatistics();
    for (let index = 0; index < 20; index += 1) {
      stats.onReceived(index + 1, 1000, 0, 0, 1000 + (index + 1));
      stats.onRendered(0, (index + 1) * 2);
    }
    const snapshot = stats.snapshot();
    expect(snapshot.publishToReceive).toEqual({ count: 20, average: 10.5, p95: 19, max: 20, min: 1 });
    expect(snapshot.receiveToRender).toMatchObject({ count: 20, average: 21, p95: 38, max: 40, min: 2 });
  });

  it('帧号出现缺口时累计核心没送到的帧数，帧号回到小数（重新上电）不算缺口', () => {
    const stats = new RenderStatistics();
    stats.onReceived(1, 0, 0, 0, 0);
    stats.onReceived(2, 16, 0, 16, 16);
    stats.onReceived(5, 66, 0, 66, 66);
    expect(stats.snapshot().coreMissed).toBe(2);
    stats.onReceived(1, 100, 0, 100, 100);
    expect(stats.snapshot().coreMissed).toBe(2);
    expect(stats.snapshot().lastFrame).toBe(1);
  });

  it('丢弃数、无效数分别计数', () => {
    const stats = new RenderStatistics();
    stats.onDropped();
    stats.onDropped();
    stats.onInvalid();
    expect(stats.snapshot()).toMatchObject({ dropped: 2, invalid: 1 });
  });

  it('核心宿主的丢帧数只算统计开始之后的增量', () => {
    const stats = new RenderStatistics();
    stats.onReceived(1, 0, 10, 0, 0);
    stats.onReceived(2, 16, 12, 16, 16);
    stats.onReceived(3, 33, 15, 33, 33);
    expect(stats.snapshot().hostDropped).toBe(5);
  });

  it('核心宿主的累计值变小（换了端口）时不出现负数', () => {
    const stats = new RenderStatistics();
    stats.onReceived(1, 0, 10, 0, 0);
    stats.onReceived(2, 16, 0, 16, 16);
    expect(stats.snapshot().hostDropped).toBe(0);
  });

  it('重置后重新开始，已有的帧不再计入', () => {
    const stats = new RenderStatistics();
    stats.onReceived(1, 0, 4, 0, 5);
    stats.onRendered(0, 2);
    stats.onDropped();
    stats.onInvalid();
    stats.reset();
    expect(stats.snapshot()).toMatchObject({ received: 0, rendered: 0, dropped: 0, invalid: 0, coreMissed: 0, hostDropped: 0, lastFrame: -1, spanMs: 0 });
    expect(stats.snapshot().publishToReceive.count).toBe(0);
    stats.onReceived(100, 1000, 9, 0, 1001);
    expect(stats.snapshot()).toMatchObject({ received: 1, coreMissed: 0, hostDropped: 0 });
  });

  it('rAF 的触发率按间隔计算，重置后重新开始', () => {
    const stats = new RenderStatistics();
    for (let index = 0; index < 61; index += 1) stats.onTick(index * (1000 / 120));
    expect(stats.snapshot().tickFps).toBeCloseTo(120, 6);
    stats.reset();
    expect(stats.snapshot().tickFps).toBe(0);
  });

  it('延迟样本有上限，超过后丢弃最早的', () => {
    const stats = new RenderStatistics();
    for (let index = 0; index < 40_100; index += 1) stats.onReceived(index + 1, 0, 0, index, index < 100 ? 1_000_000 : 1);
    expect(stats.snapshot().publishToReceive.count).toBe(40_000);
    expect(stats.snapshot().publishToReceive.max).toBe(1);
  });
});
