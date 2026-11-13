/*
 * ------------------------------------------------------------------------
 * 名称：frame-receiver.test.ts
 * 说明：帧接收逻辑的测试：帧消息的解析与校验（尺寸、字节数、像素格式、字段类型）、待画槽只保留最新一帧、像素摘要。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { LatestFrameSlot, parseFrameMessage, summarizePixels } from './frame-receiver';

/** 一条合格的帧消息，可以覆盖字段。 */
function frameMessage(overrides: Record<string, unknown> = {}): Record<string, unknown> {
  return { type: 'frame', width: 4, height: 2, format: 'rgba8', frame: 9, publishedAt: 1_700_000_000_000, hostDropped: 3, pixels: new ArrayBuffer(4 * 2 * 4), ...overrides };
}

describe('parseFrameMessage', () => {
  it('接受合格的帧并保留全部字段', () => {
    const message = frameMessage();
    const result = parseFrameMessage(message);
    expect(result.ok).toBe(true);
    if (result.ok) expect(result.value).toEqual({ width: 4, height: 2, frame: 9, publishedAt: 1_700_000_000_000, hostDropped: 3, pixels: message['pixels'] });
  });

  it('接受 256x240 的整帧', () => {
    expect(parseFrameMessage(frameMessage({ width: 256, height: 240, pixels: new ArrayBuffer(256 * 240 * 4) })).ok).toBe(true);
  });

  it('不是对象或类型不对时丢弃，没有帧号', () => {
    for (const value of [null, undefined, 'frame', 3, [], {}, { type: 'ack', frame: 1 }]) {
      expect(parseFrameMessage(value)).toEqual({ ok: false, frame: undefined });
    }
  });

  it('帧号不合格时丢弃，没有帧号', () => {
    for (const frame of [-1, 1.5, '9', Number.NaN, undefined]) expect(parseFrameMessage(frameMessage({ frame }))).toEqual({ ok: false, frame: undefined });
  });

  it('尺寸不合格时丢弃，仍给出帧号以便确认', () => {
    for (const size of [{ width: 0 }, { height: 0 }, { width: -4 }, { width: 1.5 }, { width: '4' }, { width: 4097 }, { height: 4097 }, { width: undefined }]) {
      expect(parseFrameMessage(frameMessage(size))).toEqual({ ok: false, frame: 9 });
    }
  });

  it('像素格式不对时丢弃', () => {
    for (const format of ['bgra8', 'RGBA8', '', 8, undefined]) expect(parseFrameMessage(frameMessage({ format }))).toEqual({ ok: false, frame: 9 });
  });

  it('字节数不符或不是 ArrayBuffer 时丢弃', () => {
    for (const pixels of [new ArrayBuffer(31), new ArrayBuffer(33), new ArrayBuffer(0), new Uint8Array(32), 'x', null, undefined, new SharedArrayBuffer(32)]) {
      expect(parseFrameMessage(frameMessage({ pixels }))).toEqual({ ok: false, frame: 9 });
    }
  });

  it('发布时间与丢弃数不合格时丢弃', () => {
    for (const bad of [{ publishedAt: Number.NaN }, { publishedAt: Number.POSITIVE_INFINITY }, { publishedAt: '1' }, { publishedAt: undefined }, { hostDropped: -1 }, { hostDropped: 1.5 }, { hostDropped: undefined }]) {
      expect(parseFrameMessage(frameMessage(bad))).toEqual({ ok: false, frame: 9 });
    }
  });

  it('格式错误不抛异常', () => {
    expect(() => parseFrameMessage(frameMessage({ pixels: { byteLength: 32 } }))).not.toThrow();
    expect(parseFrameMessage(frameMessage({ pixels: { byteLength: 32 } })).ok).toBe(false);
  });
});

describe('LatestFrameSlot', () => {
  it('空槽取不到帧', () => {
    expect(new LatestFrameSlot<number>().take()).toBeUndefined();
  });

  it('放入后取走，取走后槽为空', () => {
    const slot = new LatestFrameSlot<number>();
    expect(slot.offer(1)).toBeUndefined();
    expect(slot.take()).toBe(1);
    expect(slot.take()).toBeUndefined();
  });

  it('新帧顶掉还没画的旧帧，并把旧帧交还给调用方', () => {
    const slot = new LatestFrameSlot<number>();
    slot.offer(1);
    expect(slot.offer(2)).toBe(1);
    expect(slot.offer(3)).toBe(2);
    expect(slot.take()).toBe(3);
  });
});

/** 按像素列表（R、G、B、A）构造缓冲。 */
function pixelsOf(...rgba: ReadonlyArray<readonly [number, number, number, number]>): ArrayBuffer {
  const bytes = new Uint8Array(rgba.length * 4);
  rgba.forEach((pixel, index) => bytes.set(pixel, index * 4));
  return bytes.buffer;
}

describe('summarizePixels', () => {
  it('全黑的帧非黑比例为 0，只有透明度不为 0 的像素也算黑', () => {
    expect(summarizePixels(pixelsOf([0, 0, 0, 255], [0, 0, 0, 255])).nonBlackRatio).toBe(0);
    expect(summarizePixels(pixelsOf([0, 0, 0, 255])).nonBlackRatio).toBe(0);
  });

  it('按 R、G、B 任一分量不为 0 统计非黑像素', () => {
    const summary = summarizePixels(pixelsOf([1, 0, 0, 255], [0, 1, 0, 255], [0, 0, 1, 255], [0, 0, 0, 255]));
    expect(summary.nonBlackRatio).toBe(0.75);
  });

  it('内容不同的帧哈希不同，相同的帧哈希相同', () => {
    const a = summarizePixels(pixelsOf([1, 2, 3, 255], [4, 5, 6, 255]));
    const b = summarizePixels(pixelsOf([1, 2, 3, 255], [4, 5, 6, 255]));
    const c = summarizePixels(pixelsOf([1, 2, 3, 255], [4, 5, 7, 255]));
    expect(a.hash).toBe(b.hash);
    expect(a.hash).not.toBe(c.hash);
  });

  it('空缓冲的比例为 0', () => {
    expect(summarizePixels(new ArrayBuffer(0)).nonBlackRatio).toBe(0);
  });
});
