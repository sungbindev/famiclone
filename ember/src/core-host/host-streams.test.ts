/*
 * ------------------------------------------------------------------------
 * 名称：host-streams.test.ts
 * 说明：核心宿主端口的测试：输入消息（键盘、鼠标、游戏手柄）的校验、输入转给回调、帧消息的格式与发送、换端口时关闭旧端口、端口关闭或发送失败时丢帧。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { HostStreams, parseAckMessage, parseInputMessage, type HostPort } from './host-streams';

/** 假端口：记录发送的消息，可以模拟收到消息与关闭。 */
class FakePort implements HostPort {
  readonly posted: unknown[] = [];
  started = false;
  closed = false;
  failing = false;
  private messageListener: ((event: { readonly data: unknown }) => void) | undefined;
  private closeListener: (() => void) | undefined;

  postMessage(message: unknown): void {
    if (this.failing) throw new Error('端口已关闭');
    this.posted.push(message);
  }
  on(event: 'message', listener: (event: { readonly data: unknown }) => void): unknown;
  on(event: 'close', listener: () => void): unknown;
  on(event: 'message' | 'close', listener: ((event: { readonly data: unknown }) => void) | (() => void)): unknown {
    if (event === 'message') this.messageListener = listener as (event: { readonly data: unknown }) => void;
    else this.closeListener = listener as () => void;
    return this;
  }
  start(): void {
    this.started = true;
  }
  close(): void {
    this.closed = true;
  }
  receive(data: unknown): void {
    this.messageListener?.({ data });
  }
  remoteClose(): void {
    this.closeListener?.();
  }
}

describe('parseInputMessage', () => {
  const mouse = { type: 'mouse', kind: 'move', x: 10.5, y: -3, dx: 1, dy: 0, buttons: 5, wheel: 0 };
  const gamepad = { type: 'gamepad', index: 1, id: 'Test Pad', connected: true, buttons: [true, false], axes: [0.5, -1] };

  it('接受合格的键盘、鼠标、游戏手柄消息', () => {
    expect(parseInputMessage({ type: 'key', code: 'KeyW', down: true })).toEqual({ type: 'key', code: 'KeyW', down: true });
    expect(parseInputMessage({ type: 'key', code: 'Numpad1', down: false })).toEqual({ type: 'key', code: 'Numpad1', down: false });
    expect(parseInputMessage(mouse)).toEqual(mouse);
    expect(parseInputMessage(gamepad)).toEqual(gamepad);
    expect(parseInputMessage({ ...gamepad, connected: false, buttons: [], axes: [] })).toEqual({ ...gamepad, connected: false, buttons: [], axes: [] });
  });

  it('丢弃格式不对的消息', () => {
    const bad: unknown[] = [
      null,
      undefined,
      'key',
      42,
      [],
      {},
      { type: 'buttons', port: 0, buttons: 1 },
      { type: 'key', code: 'KeyW' },
      { type: 'key', code: 'KeyW', down: 1 },
      { type: 'key', code: '', down: true },
      { type: 'key', code: 'Key W', down: true },
      { type: 'key', code: 'K'.repeat(33), down: true },
      { type: 'key', code: 5, down: true },
      { ...mouse, kind: 'drag' },
      { ...mouse, x: Number.NaN },
      { ...mouse, y: '1' },
      { ...mouse, dx: Number.POSITIVE_INFINITY },
      { ...mouse, x: 2_000_000 },
      { ...mouse, buttons: 8 },
      { ...mouse, buttons: -1 },
      { ...mouse, buttons: 1.5 },
      { ...mouse, wheel: undefined },
      { ...gamepad, index: 16 },
      { ...gamepad, index: -1 },
      { ...gamepad, id: 1 },
      { ...gamepad, id: 'x'.repeat(257) },
      { type: 'gamepad', index: 1, connected: true, buttons: [], axes: [] },
      { ...gamepad, connected: 1 },
      { ...gamepad, buttons: 'x' },
      { ...gamepad, buttons: [1] },
      { ...gamepad, buttons: Array.from({ length: 33 }, () => true) },
      { ...gamepad, axes: [2] },
      { ...gamepad, axes: ['0'] },
      { ...gamepad, axes: Array.from({ length: 17 }, () => 0) }
    ];
    for (const value of bad) expect(parseInputMessage(value)).toBeUndefined();
  });

  it('只保留约定的字段', () => {
    expect(parseInputMessage({ type: 'key', code: 'KeyW', down: true, extra: '多余' })).toEqual({ type: 'key', code: 'KeyW', down: true });
    expect(parseInputMessage({ ...mouse, extra: 1 })).toEqual(mouse);
  });
});

describe('parseAckMessage', () => {
  it('接受合格的确认', () => {
    expect(parseAckMessage({ type: 'ack', frame: 0 })).toEqual({ type: 'ack', frame: 0 });
    expect(parseAckMessage({ type: 'ack', frame: 123, extra: '多余' })).toEqual({ type: 'ack', frame: 123 });
  });

  it('丢弃格式不对的确认', () => {
    const bad: unknown[] = [null, undefined, 'ack', 7, [], {}, { type: 'frame', frame: 1 }, { type: 'ack' }, { type: 'ack', frame: -1 }, { type: 'ack', frame: 1.5 }, { type: 'ack', frame: '1' }, { type: 'ack', frame: Number.NaN }];
    for (const value of bad) expect(parseAckMessage(value)).toBeUndefined();
  });
});

/** 发一帧，像素内容无关紧要。 */
function send(streams: HostStreams, frame: number): void {
  streams.sendFrame(new ArrayBuffer(4), 1, 1, frame, 0);
}

describe('HostStreams 的背压', () => {
  it('在途帧达到上限后丢弃新帧并计数，没有确认前不再发送', () => {
    const frames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    for (let frame = 1; frame <= 5; frame += 1) send(streams, frame);
    expect(frames.posted).toHaveLength(2);
    expect(streams.framesInFlight).toBe(2);
    expect(streams.backpressureDropped).toBe(3);
  });

  it('收到确认后释放名额，之后的帧带着累计的丢弃数发出', () => {
    const frames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    for (let frame = 1; frame <= 4; frame += 1) send(streams, frame);
    frames.receive({ type: 'ack', frame: 1 });
    expect(streams.framesInFlight).toBe(1);
    send(streams, 5);
    expect(frames.posted).toHaveLength(3);
    expect(frames.posted[2]).toMatchObject({ frame: 5, hostDropped: 2 });
  });

  it('确认可以乱序到达，重复或没发过的确认被忽略', () => {
    const frames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    send(streams, 1);
    send(streams, 2);
    frames.receive({ type: 'ack', frame: 2 });
    frames.receive({ type: 'ack', frame: 2 });
    frames.receive({ type: 'ack', frame: 99 });
    expect(streams.framesInFlight).toBe(1);
    frames.receive({ type: 'ack', frame: 1 });
    expect(streams.framesInFlight).toBe(0);
  });

  it('格式不对的确认被丢弃，不释放名额', () => {
    const frames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    send(streams, 1);
    frames.receive({ type: 'ack', frame: '1' });
    frames.receive('垃圾');
    expect(streams.framesInFlight).toBe(1);
  });

  it('渲染进程卡住（一直不确认）时队列里最多两帧，恢复确认后继续发送', () => {
    const frames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    for (let frame = 1; frame <= 600; frame += 1) send(streams, frame);
    expect(frames.posted).toHaveLength(2);
    expect(streams.backpressureDropped).toBe(598);
    frames.receive({ type: 'ack', frame: 1 });
    frames.receive({ type: 'ack', frame: 2 });
    send(streams, 601);
    expect(frames.posted).toHaveLength(3);
  });

  it('换端口后在途数与丢弃数清零', () => {
    const streams = new HostStreams(() => undefined);
    streams.attach([new FakePort(), new FakePort()]);
    for (let frame = 1; frame <= 4; frame += 1) send(streams, frame);
    const frames = new FakePort();
    streams.attach([frames, new FakePort()]);
    expect(streams.framesInFlight).toBe(0);
    expect(streams.backpressureDropped).toBe(0);
    send(streams, 5);
    expect(frames.posted[0]).toMatchObject({ frame: 5, hostDropped: 0 });
  });

  it('旧端口迟到的确认不影响新端口', () => {
    const oldFrames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([oldFrames, new FakePort()]);
    send(streams, 1);
    const frames = new FakePort();
    streams.attach([frames, new FakePort()]);
    send(streams, 1);
    oldFrames.receive({ type: 'ack', frame: 1 });
    expect(streams.framesInFlight).toBe(1);
  });

  it('发送失败的帧不占用在途名额', () => {
    const frames = new FakePort();
    frames.failing = true;
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    send(streams, 1);
    expect(streams.framesInFlight).toBe(0);
  });
});

describe('HostStreams', () => {
  it('按帧格式发送画面，像素原样交给端口', () => {
    const [frames, input] = [new FakePort(), new FakePort()];
    const streams = new HostStreams(() => undefined);
    expect(streams.attach([frames, input])).toBe(true);
    expect(frames.started).toBe(true);
    expect(input.started).toBe(true);
    const pixels = new ArrayBuffer(256 * 240 * 4);
    streams.sendFrame(pixels, 256, 240, 7, 1_700_000_000_123);
    expect(frames.posted).toEqual([{ type: 'frame', width: 256, height: 240, format: 'rgba8', frame: 7, publishedAt: 1_700_000_000_123, hostDropped: 0, pixels }]);
    expect((frames.posted[0] as { pixels: ArrayBuffer }).pixels).toBe(pixels);
    expect(input.posted).toHaveLength(0);
  });

  it('端口个数不对时不采用', () => {
    const streams = new HostStreams(() => undefined);
    expect(streams.attach([])).toBe(false);
    expect(streams.attach([new FakePort()])).toBe(false);
    expect(streams.attach([new FakePort(), new FakePort(), new FakePort()])).toBe(false);
  });

  it('没有端口时丢弃帧', () => {
    const streams = new HostStreams(() => undefined);
    expect(() => streams.sendFrame(new ArrayBuffer(4), 1, 1, 1, 0)).not.toThrow();
  });

  it('输入端口收到的输入校验后交给回调，格式不对的丢弃', () => {
    const onInput = vi.fn();
    const input = new FakePort();
    new HostStreams(onInput).attach([new FakePort(), input]);
    input.receive({ type: 'key', code: 'Space', down: true });
    input.receive({ type: 'key', code: 'Sp ace', down: true });
    input.receive('垃圾');
    expect(onInput).toHaveBeenCalledTimes(1);
    expect(onInput).toHaveBeenCalledWith({ type: 'key', code: 'Space', down: true });
  });

  it('换端口时关闭旧端口，之后只往新端口发帧', () => {
    const [oldFrames, oldInput] = [new FakePort(), new FakePort()];
    const [newFrames, newInput] = [new FakePort(), new FakePort()];
    const streams = new HostStreams(() => undefined);
    streams.attach([oldFrames, oldInput]);
    streams.attach([newFrames, newInput]);
    expect(oldFrames.closed).toBe(true);
    expect(oldInput.closed).toBe(true);
    streams.sendFrame(new ArrayBuffer(4), 1, 1, 1, 0);
    expect(oldFrames.posted).toHaveLength(0);
    expect(newFrames.posted).toHaveLength(1);
  });

  it('个数不对的端口不会关闭现有的端口', () => {
    const [frames, input] = [new FakePort(), new FakePort()];
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, input]);
    streams.attach([new FakePort()]);
    expect(frames.closed).toBe(false);
    expect(input.closed).toBe(false);
  });

  it('画面端口被对端关闭后不再发送', () => {
    const frames = new FakePort();
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    frames.remoteClose();
    streams.sendFrame(new ArrayBuffer(4), 1, 1, 1, 0);
    expect(frames.posted).toHaveLength(0);
  });

  it('发送失败时丢弃这一帧，不抛出', () => {
    const frames = new FakePort();
    frames.failing = true;
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    expect(() => streams.sendFrame(new ArrayBuffer(4), 1, 1, 1, 0)).not.toThrow();
  });

  it('detach 关闭两个端口，重复调用与关闭失败都不抛出', () => {
    const [frames, input] = [new FakePort(), new FakePort()];
    input.close = () => {
      throw new Error('已经关闭');
    };
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, input]);
    expect(() => streams.detach()).not.toThrow();
    expect(frames.closed).toBe(true);
    expect(() => streams.detach()).not.toThrow();
  });
});
