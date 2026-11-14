/*
 * ------------------------------------------------------------------------
 * 名称：worker-messages.test.ts
 * 说明：主线程与渲染 Worker 之间消息校验的测试：各类消息的合格与不合格情形。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { afterEach, describe, expect, it, vi } from 'vitest';
import { MAX_CANVAS_DIMENSION, MAX_CROP_LINES, nextScreenView, parseWorkerReply, parseWorkerRequest, type ScreenView } from './worker-messages';

/** 测试用的假画布与假端口类，只用来通过 instanceof 检查。 */
class FakeOffscreenCanvas {
  readonly kind = 'canvas';
}
class FakeMessagePort {
  readonly kind = 'port';
}

afterEach(() => {
  vi.unstubAllGlobals();
});

describe('parseWorkerRequest', () => {
  it('接受合格的版面消息', () => {
    const view = { width: 1280, height: 960, aspect: '4:3', scaling: 'integer', cropLines: 8 };
    expect(parseWorkerRequest({ type: 'view', view })).toEqual({ type: 'view', view });
    expect(parseWorkerRequest({ type: 'view', view: { width: 0, height: 0, aspect: '8:7', scaling: 'fit', cropLines: 0 } })).toBeDefined();
  });

  it('丢弃不合格的版面消息', () => {
    const base = { width: 100, height: 100, aspect: '4:3', scaling: 'fit', cropLines: 0 };
    const bad: unknown[] = [
      undefined,
      null,
      { ...base, width: -1 },
      { ...base, height: 1.5 },
      { ...base, width: MAX_CANVAS_DIMENSION + 1 },
      { ...base, width: '100' },
      { ...base, aspect: '16:9' },
      { ...base, scaling: 'stretch' },
      { width: 100, height: 100, aspect: '4:3', scaling: 'fit' },
      { ...base, cropLines: -1 },
      { ...base, cropLines: 1.5 },
      { ...base, cropLines: MAX_CROP_LINES + 1 },
      { ...base, cropLines: '8' }
    ];
    for (const view of bad) expect(parseWorkerRequest({ type: 'view', view })).toBeUndefined();
  });

  it('接受合格的诊断请求，丢弃不合格的', () => {
    expect(parseWorkerRequest({ type: 'diagnostics', id: 3, reset: true })).toEqual({ type: 'diagnostics', id: 3, reset: true });
    for (const message of [{ id: -1, reset: false }, { id: 1.5, reset: false }, { id: '1', reset: false }, { id: 1, reset: 'yes' }, { id: 1 }]) {
      expect(parseWorkerRequest({ type: 'diagnostics', ...message })).toBeUndefined();
    }
  });

  it('detach 不带参数', () => {
    expect(parseWorkerRequest({ type: 'detach' })).toEqual({ type: 'detach' });
  });

  it('init 与 attach 只接受画布与端口对象', () => {
    vi.stubGlobal('OffscreenCanvas', FakeOffscreenCanvas);
    vi.stubGlobal('MessagePort', FakeMessagePort);
    const canvas = new FakeOffscreenCanvas();
    const port = new FakeMessagePort();
    expect(parseWorkerRequest({ type: 'init', canvas })).toMatchObject({ type: 'init' });
    expect(parseWorkerRequest({ type: 'attach', port })).toMatchObject({ type: 'attach' });
    expect(parseWorkerRequest({ type: 'init', canvas: {} })).toBeUndefined();
    expect(parseWorkerRequest({ type: 'init' })).toBeUndefined();
    expect(parseWorkerRequest({ type: 'attach', port: {} })).toBeUndefined();
    expect(parseWorkerRequest({ type: 'attach', port: 'x' })).toBeUndefined();
  });

  it('运行环境没有画布与端口类时 init、attach 一律丢弃', () => {
    vi.stubGlobal('OffscreenCanvas', undefined);
    vi.stubGlobal('MessagePort', undefined);
    expect(parseWorkerRequest({ type: 'init', canvas: {} })).toBeUndefined();
    expect(parseWorkerRequest({ type: 'attach', port: {} })).toBeUndefined();
  });

  it('未知类型与非对象一律丢弃', () => {
    for (const value of [null, undefined, 'view', 3, [], {}, { type: 'run' }, { type: 5 }]) expect(parseWorkerRequest(value)).toBeUndefined();
  });
});

describe('parseWorkerReply', () => {
  it('接受 ready、failed 与诊断回复', () => {
    expect(parseWorkerReply({ type: 'ready' })).toEqual({ type: 'ready' });
    expect(parseWorkerReply({ type: 'failed', message: '不支持' })).toEqual({ type: 'failed', message: '不支持' });
    expect(parseWorkerReply({ type: 'diagnostics', id: 2, data: { received: 1 } })).toMatchObject({ type: 'diagnostics', id: 2 });
  });

  it('丢弃格式不对的回复', () => {
    for (const value of [null, undefined, 'ready', {}, { type: 'failed' }, { type: 'failed', message: 5 }, { type: 'diagnostics', id: '1', data: {} }, { type: 'diagnostics', id: 1 }, { type: 'diagnostics', id: 1, data: null }, { type: 'other' }]) {
      expect(parseWorkerReply(value)).toBeUndefined();
    }
  });
});


describe('nextScreenView', () => {
  const current: ScreenView = { width: 1280, height: 960, aspect: '4:3', scaling: 'fit', cropLines: 0 };

  it('尺寸合格时采用新版面', () => {
    const incoming: ScreenView = { width: 800, height: 600, aspect: '8:7', scaling: 'integer', cropLines: 8 };
    expect(nextScreenView(current, incoming)).toEqual(incoming);
  });

  it('宽或高为 0（页面隐藏）时保持上一次的尺寸，画面比例、缩放方式与裁剪行数照常更新', () => {
    expect(nextScreenView(current, { width: 0, height: 0, aspect: '8:7', scaling: 'integer', cropLines: 8 })).toEqual({ width: 1280, height: 960, aspect: '8:7', scaling: 'integer', cropLines: 8 });
    expect(nextScreenView(current, { width: 0, height: 600, aspect: '4:3', scaling: 'fit', cropLines: 0 })).toEqual(current);
    expect(nextScreenView(current, { width: 800, height: 0, aspect: '4:3', scaling: 'fit', cropLines: 0 })).toEqual(current);
  });

  it('一直没有真实尺寸时保持 0', () => {
    const empty: ScreenView = { width: 0, height: 0, aspect: '4:3', scaling: 'fit', cropLines: 0 };
    expect(nextScreenView(empty, { ...empty })).toEqual(empty);
  });
});