/*
 * ------------------------------------------------------------------------
 * 名称：frame-session.test.ts
 * 说明：帧接收会话的测试：合格的帧进入待画槽、来不及画的旧帧被顶掉并确认、无效帧丢弃计数、画完确认、每帧只确认一次、统计与诊断。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { FrameSession } from './frame-session';

/** 构造 4x2 的小帧消息，像素用 fill 填充（R 分量）。 */
function frameMessage(frame: number, options: { publishedAt?: number; hostDropped?: number; fill?: number } = {}): Record<string, unknown> {
  const pixels = new Uint8Array(4 * 2 * 4);
  for (let index = 0; index < pixels.length; index += 4) pixels[index] = options.fill ?? 0;
  return { type: 'frame', width: 4, height: 2, format: 'rgba8', frame, publishedAt: options.publishedAt ?? 1000, hostDropped: options.hostDropped ?? 0, pixels: pixels.buffer };
}

/** 建会话：时钟由测试推进，确认记录在数组里。 */
function setup() {
  const acks: number[] = [];
  const clock = { now: 0, wall: 1000 };
  const session = new FrameSession((frame) => acks.push(frame), { now: () => clock.now, wallNow: () => clock.wall });
  return { session, acks, clock };
}

describe('FrameSession：接收与待画', () => {
  it('合格的帧进入待画槽，取走后槽为空', () => {
    const { session } = setup();
    session.receive(frameMessage(1));
    expect(session.takePending()?.frame.frame).toBe(1);
    expect(session.takePending()).toBeUndefined();
    expect(session.statistics.snapshot()).toMatchObject({ received: 1, dropped: 0, invalid: 0 });
  });

  it('来不及画时新帧顶掉旧帧：旧帧计入丢弃并立即确认，只画最新一帧', () => {
    const { session, acks } = setup();
    session.receive(frameMessage(1));
    session.receive(frameMessage(2));
    session.receive(frameMessage(3));
    expect(acks).toEqual([1, 2]);
    expect(session.statistics.snapshot()).toMatchObject({ received: 3, dropped: 2 });
    expect(session.takePending()?.frame.frame).toBe(3);
  });

  it('记录收到的时间，画完时算出收到到画出的延迟', () => {
    const { session, clock } = setup();
    clock.now = 100;
    session.receive(frameMessage(1));
    clock.now = 112;
    const pending = session.takePending();
    expect(pending?.receivedAt).toBe(100);
    if (pending !== undefined) session.completeRender(pending);
    expect(session.statistics.snapshot().receiveToRender).toMatchObject({ count: 1, average: 12, max: 12 });
  });

  it('核心发布到收到的延迟取墙上时钟之差', () => {
    const { session, clock } = setup();
    clock.wall = 1007;
    session.receive(frameMessage(1, { publishedAt: 1000 }));
    expect(session.statistics.snapshot().publishToReceive).toMatchObject({ count: 1, average: 7 });
  });
});

describe('FrameSession：确认', () => {
  it('画完一帧后确认它的帧号', () => {
    const { session, acks } = setup();
    session.receive(frameMessage(5));
    const pending = session.takePending();
    expect(acks).toEqual([]);
    if (pending !== undefined) session.completeRender(pending);
    expect(acks).toEqual([5]);
    expect(session.statistics.snapshot().rendered).toBe(1);
  });

  it('每一帧恰好确认一次：画出的、被顶掉的各一次', () => {
    const { session, acks } = setup();
    for (let frame = 1; frame <= 10; frame += 1) {
      session.receive(frameMessage(frame));
      if (frame % 3 === 0) {
        const pending = session.takePending();
        if (pending !== undefined) session.completeRender(pending);
      }
    }
    const pending = session.takePending();
    if (pending !== undefined) session.completeRender(pending);
    expect(acks.toSorted((a, b) => a - b)).toEqual([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
  });

  it('格式不符但帧号可读的帧计入无效并确认，帧号不可读的只计数', () => {
    const { session, acks } = setup();
    session.receive({ ...frameMessage(7), pixels: new ArrayBuffer(3) });
    session.receive({ type: 'frame', frame: 'x' });
    session.receive('垃圾');
    session.receive(null);
    expect(acks).toEqual([7]);
    expect(session.statistics.snapshot()).toMatchObject({ received: 0, invalid: 4 });
    expect(session.takePending()).toBeUndefined();
  });

  it('无效帧不会顶掉待画的有效帧', () => {
    const { session } = setup();
    session.receive(frameMessage(1));
    session.receive({ ...frameMessage(2), format: 'bgra8' });
    expect(session.takePending()?.frame.frame).toBe(1);
  });
});

describe('FrameSession：诊断', () => {
  it('还没收到帧时像素摘要为 -1，尺寸为 0', () => {
    const { session } = setup();
    expect(session.diagnostics()).toMatchObject({ lastFrameNonBlackRatio: -1, lastFrameHash: -1, frameWidth: 0, frameHeight: 0, received: 0 });
  });

  it('给出最近收到的一帧的尺寸、非黑像素比例与哈希，换了内容哈希就变', () => {
    const { session } = setup();
    session.receive(frameMessage(1, { fill: 0 }));
    const black = session.diagnostics();
    expect(black).toMatchObject({ frameWidth: 4, frameHeight: 2, lastFrameNonBlackRatio: 0, lastFrame: 1 });
    session.receive(frameMessage(2, { fill: 9 }));
    const colored = session.diagnostics();
    expect(colored.lastFrameNonBlackRatio).toBe(1);
    expect(colored.lastFrameHash).not.toBe(black.lastFrameHash);
  });

  it('没被画出的最新一帧也能读到摘要（取最近收到的，不是最近画出的）', () => {
    const { session } = setup();
    session.receive(frameMessage(1, { fill: 0 }));
    const pending = session.takePending();
    if (pending !== undefined) session.completeRender(pending);
    session.receive(frameMessage(2, { fill: 5 }));
    expect(session.diagnostics().lastFrameNonBlackRatio).toBe(1);
  });

  it('重置统计后重新开始，但最近一帧的摘要仍可读', () => {
    const { session } = setup();
    session.receive(frameMessage(1, { fill: 5 }));
    session.resetStatistics();
    expect(session.diagnostics()).toMatchObject({ received: 0, lastFrameNonBlackRatio: 1 });
  });
});
