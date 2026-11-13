/*
 * ------------------------------------------------------------------------
 * 名称：host-streams.test.ts
 * 说明：核心宿主端口的测试：输入消息的校验、按键转给回调、帧消息的格式与发送、换端口时关闭旧端口、端口关闭或发送失败时丢帧。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { HostStreams, parseInputMessage, type HostPort } from './host-streams';

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
  it('接受合格的按键消息', () => {
    expect(parseInputMessage({ type: 'buttons', port: 0, buttons: 0 })).toEqual({ type: 'buttons', port: 0, buttons: 0 });
    expect(parseInputMessage({ type: 'buttons', port: 1, buttons: 255 })).toEqual({ type: 'buttons', port: 1, buttons: 255 });
  });

  it('丢弃格式不对的消息', () => {
    const bad: unknown[] = [
      null,
      undefined,
      'buttons',
      42,
      [],
      {},
      { type: 'keys', port: 0, buttons: 1 },
      { type: 'buttons', port: 2, buttons: 1 },
      { type: 'buttons', port: -1, buttons: 1 },
      { type: 'buttons', port: 0.5, buttons: 1 },
      { type: 'buttons', port: '0', buttons: 1 },
      { type: 'buttons', port: 0, buttons: 256 },
      { type: 'buttons', port: 0, buttons: -1 },
      { type: 'buttons', port: 0, buttons: 1.5 },
      { type: 'buttons', port: 0, buttons: Number.NaN },
      { type: 'buttons', port: 0, buttons: '1' },
      { type: 'buttons', port: 0 }
    ];
    for (const value of bad) expect(parseInputMessage(value)).toBeUndefined();
  });

  it('只保留约定的字段', () => {
    expect(parseInputMessage({ type: 'buttons', port: 0, buttons: 1, extra: '多余' })).toEqual({ type: 'buttons', port: 0, buttons: 1 });
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
    streams.sendFrame(pixels, 256, 240, 7);
    expect(frames.posted).toEqual([{ type: 'frame', width: 256, height: 240, format: 'rgba8', frame: 7, pixels }]);
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
    expect(() => streams.sendFrame(new ArrayBuffer(4), 1, 1, 1)).not.toThrow();
  });

  it('输入端口收到的按键校验后交给回调，格式不对的丢弃', () => {
    const onInput = vi.fn();
    const input = new FakePort();
    new HostStreams(onInput).attach([new FakePort(), input]);
    input.receive({ type: 'buttons', port: 0, buttons: 9 });
    input.receive({ type: 'buttons', port: 5, buttons: 9 });
    input.receive('垃圾');
    expect(onInput).toHaveBeenCalledTimes(1);
    expect(onInput).toHaveBeenCalledWith({ type: 'buttons', port: 0, buttons: 9 });
  });

  it('换端口时关闭旧端口，之后只往新端口发帧', () => {
    const [oldFrames, oldInput] = [new FakePort(), new FakePort()];
    const [newFrames, newInput] = [new FakePort(), new FakePort()];
    const streams = new HostStreams(() => undefined);
    streams.attach([oldFrames, oldInput]);
    streams.attach([newFrames, newInput]);
    expect(oldFrames.closed).toBe(true);
    expect(oldInput.closed).toBe(true);
    streams.sendFrame(new ArrayBuffer(4), 1, 1, 1);
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
    streams.sendFrame(new ArrayBuffer(4), 1, 1, 1);
    expect(frames.posted).toHaveLength(0);
  });

  it('发送失败时丢弃这一帧，不抛出', () => {
    const frames = new FakePort();
    frames.failing = true;
    const streams = new HostStreams(() => undefined);
    streams.attach([frames, new FakePort()]);
    expect(() => streams.sendFrame(new ArrayBuffer(4), 1, 1, 1)).not.toThrow();
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
