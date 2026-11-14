/*
 * ------------------------------------------------------------------------
 * 名称：render-worker.test.ts
 * 说明：渲染 Worker 对版面的处理测试：页面隐藏时 ResizeObserver 会给出 0×0，Worker 要忽略这个尺寸、保持上一次的画布尺寸，再次显示后用真实尺寸；版面变化时立即重画上一帧。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 Worker 全局对象与假的 WebGL 渲染器加载 render-worker.ts，经消息监听器发送消息；不依赖真实的 WebGL。
 * ------------------------------------------------------------------------
 */

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

/** 渲染器的假实现：记下每次调整画布尺寸。 */
const renderer = vi.hoisted(() => ({ resize: vi.fn(), draw: vi.fn() }));

vi.mock('./webgl-renderer', () => ({
  WebGlFrameRenderer: class {
    readonly size = { width: 0, height: 0 };
    resize = renderer.resize;
    draw = renderer.draw;
  }
}));

/** 假的离屏画布：只用来通过 init 消息的类型检查。 */
class FakeOffscreenCanvas {
  readonly kind = 'canvas';
}

/** Worker 全局对象的假实现。 */
interface FakeScope {
  listener?: (event: MessageEvent) => void;
  addEventListener(type: 'message', listener: (event: MessageEvent) => void): void;
  postMessage: ReturnType<typeof vi.fn>;
  requestAnimationFrame: ReturnType<typeof vi.fn>;
}

let scope: FakeScope;

/** 向 Worker 发一条消息。 */
function send(data: unknown): void {
  scope.listener?.({ data } as MessageEvent);
}

/** 版面消息。 */
function view(width: number, height: number, aspect: '4:3' | '8:7' = '4:3', cropLines = 0): unknown {
  return { type: 'view', view: { width, height, aspect, scaling: 'fit', cropLines } };
}

/** 假的消息端口：记下 Worker 挂上去的监听函数。 */
class FakeMessagePort {
  listener?: (event: MessageEvent) => void;
  addEventListener(_type: string, listener: (event: MessageEvent) => void): void {
    this.listener = listener;
  }
  start = vi.fn();
  close = vi.fn();
  postMessage = vi.fn();
}

beforeEach(async () => {
  vi.resetModules();
  renderer.resize.mockClear();
  renderer.draw.mockClear();
  scope = {
    addEventListener(_type, listener) {
      this.listener = listener;
    },
    postMessage: vi.fn(),
    requestAnimationFrame: vi.fn()
  };
  vi.stubGlobal('self', scope);
  vi.stubGlobal('OffscreenCanvas', FakeOffscreenCanvas);
  vi.stubGlobal('MessagePort', FakeMessagePort);
  await import('./render-worker');
  send({ type: 'init', canvas: new FakeOffscreenCanvas() });
});

afterEach(() => {
  vi.unstubAllGlobals();
});

describe('渲染 Worker 的版面', () => {
  it('初始化后通知主线程已就绪', () => {
    expect(scope.postMessage).toHaveBeenCalledWith({ type: 'ready' });
  });

  it('采用真实尺寸调整画布', () => {
    send(view(1280, 960));
    expect(renderer.resize).toHaveBeenLastCalledWith(1280, 960);
  });

  it('收到 0×0（页面隐藏）时忽略，保持上一次的画布尺寸', () => {
    send(view(1280, 960));
    renderer.resize.mockClear();
    send(view(0, 0));
    expect(renderer.resize).toHaveBeenCalledTimes(1);
    expect(renderer.resize).toHaveBeenLastCalledWith(1280, 960);
    expect(renderer.resize).not.toHaveBeenCalledWith(0, 0);
  });

  it('只有宽或只有高为 0 时同样忽略', () => {
    send(view(800, 600));
    send(view(0, 600));
    send(view(800, 0));
    expect(renderer.resize).toHaveBeenLastCalledWith(800, 600);
    expect(renderer.resize).not.toHaveBeenCalledWith(0, 600);
    expect(renderer.resize).not.toHaveBeenCalledWith(800, 0);
  });

  it('页面重新显示后用新的真实尺寸', () => {
    send(view(1280, 960));
    send(view(0, 0));
    send(view(1024, 768));
    expect(renderer.resize).toHaveBeenLastCalledWith(1024, 768);
  });

  it('页面隐藏期间画面比例的变化不丢：尺寸保持，之后的真实尺寸仍用新比例', () => {
    send(view(1280, 960));
    send(view(0, 0, '8:7'));
    expect(renderer.resize).toHaveBeenLastCalledWith(1280, 960);
    send(view(1280, 960, '8:7'));
    expect(renderer.resize).toHaveBeenLastCalledWith(1280, 960);
  });

  it('改变画布尺寸会清空画布，所以版面变化时立即用上一帧重画，不等下一次 rAF', () => {
    const port = new FakeMessagePort();
    send({ type: 'attach', port });
    send(view(1280, 960));
    port.listener?.({ data: { type: 'frame', width: 4, height: 2, format: 'rgba8', frame: 1, publishedAt: 1000, hostDropped: 0, pixels: new ArrayBuffer(32) } } as MessageEvent);
    const tick = scope.requestAnimationFrame.mock.calls[0]?.[0] as () => void;
    tick();
    expect(renderer.draw).toHaveBeenCalledTimes(1);
    renderer.draw.mockClear();
    send(view(800, 600));
    expect(renderer.draw).toHaveBeenCalledTimes(1);
    expect(renderer.draw.mock.calls[0]?.[0]).toBeUndefined();
  });

  it('上下裁掉 8 行时按 256x224 计算画面矩形，并把裁剪行数交给渲染器', () => {
    const port = new FakeMessagePort();
    send({ type: 'attach', port });
    send(view(1280, 960, '8:7', 8));
    port.listener?.({ data: { type: 'frame', width: 256, height: 240, format: 'rgba8', frame: 1, publishedAt: 1000, hostDropped: 0, pixels: new ArrayBuffer(256 * 240 * 4) } } as MessageEvent);
    const tick = scope.requestAnimationFrame.mock.calls[0]?.[0] as () => void;
    tick();
    // 256x224 按 8:7 的宽高比约 1.306，画布 1280x960 的宽高比 1.333 更宽：高度铺满 960，宽度 1254，左右各留 13。
    expect(renderer.draw.mock.calls.at(-1)?.[1]).toEqual({ x: 13, y: 0, width: 1254, height: 960 });
    expect(renderer.draw.mock.calls.at(-1)?.[2]).toBe(8);
    renderer.draw.mockClear();
    send(view(1280, 960, '8:7', 0));
    // 不裁时 256x240：宽高比 1.219，宽度 1170。
    expect(renderer.draw.mock.calls.at(-1)?.[1]).toEqual({ x: 55, y: 0, width: 1170, height: 960 });
    expect(renderer.draw.mock.calls.at(-1)?.[2]).toBe(0);
  });
});
