/*
 * ------------------------------------------------------------------------
 * 名称：render-worker.ts
 * 说明：渲染 Worker 入口：接管画布与画面端口，在 Worker 里收帧、只保留最新一帧、由 requestAnimationFrame 驱动用 WebGL 画出、每帧回发确认，并统计延迟与丢帧。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：Worker 与界面层同属渲染侧，不访问 Node 与 Electron；主线程发来的消息先按类型校验再处理，格式不对的丢弃。
 *       诊断（统计与最近一帧的像素摘要）经 'diagnostics' 消息读出，只在开发期由画面组件挂到 window 上使用。
 *       渲染循环一直运行（页面可见时 rAF 才会触发）：有新帧就画新帧；版面变了（改变画布尺寸会清空画布）就在收到版面消息时立即重画上一帧。
 * ------------------------------------------------------------------------
 */

import { FrameSession, type PendingFrame } from './frame-session';
import { computeScreenRect, DEFAULT_SCREEN_ASPECT, DEFAULT_SCREEN_SCALING } from './screen-layout';
import { WebGlFrameRenderer } from './webgl-renderer';
import { nextScreenView, parseWorkerRequest, type ScreenView, type WorkerReply, type WorkerRequest } from './worker-messages';

/** Worker 全局对象里用到的部分（项目的 TypeScript 库只有 DOM，没有 WebWorker）。 */
interface RenderWorkerScope {
  addEventListener(type: 'message', listener: (event: MessageEvent) => void): void;
  postMessage(message: WorkerReply): void;
  requestAnimationFrame(callback: () => void): number;
}

const scope = self as unknown as RenderWorkerScope;

const TEXT = {
  rendererFailed: '无法初始化画面渲染'
} as const;

/** 画面端口：核心发帧，这里回确认。 */
let framePort: MessagePort | undefined;
/** 帧会话：接收、待画槽、统计；确认经画面端口发回核心。 */
const session = new FrameSession(
  (frame) => {
    try {
      framePort?.postMessage({ type: 'ack', frame });
    } catch {
      // 端口已关闭，核心一侧会随端口一起清理。
    }
  },
  { now: () => performance.now(), wallNow: () => Date.now() }
);
let renderer: WebGlFrameRenderer | undefined;
let view: ScreenView = { width: 0, height: 0, aspect: DEFAULT_SCREEN_ASPECT, scaling: DEFAULT_SCREEN_SCALING };
/** 最近一次画出的帧的尺寸：版面变化时按它算矩形。 */
let drawnSize: { width: number; height: number } | undefined;
let loopRunning = false;

/** 画一帧；没有新帧时按上一帧的尺寸重画（纹理里还保留着上一帧）。 */
function paint(pending: PendingFrame | undefined): void {
  if (renderer === undefined) return;
  const size = pending === undefined ? drawnSize : { width: pending.frame.width, height: pending.frame.height };
  if (size === undefined) return;
  const rect = computeScreenRect(view.width, view.height, size.width, size.height, view.aspect, view.scaling);
  renderer.draw(pending?.frame, rect);
  drawnSize = size;
}

/** 渲染循环：每次浏览器要画下一帧时，画待画的最新一帧。 */
function tick(): void {
  scope.requestAnimationFrame(tick);
  session.statistics.onTick(performance.now());
  const pending = session.takePending();
  if (pending !== undefined) {
    paint(pending);
    session.completeRender(pending);
  }
}

/** 启动渲染循环（只启动一次）。 */
function startLoop(): void {
  if (loopRunning) return;
  loopRunning = true;
  scope.requestAnimationFrame(tick);
}

/** 初始化渲染器。 */
function initialize(canvas: OffscreenCanvas): void {
  try {
    renderer = new WebGlFrameRenderer(canvas);
    renderer.resize(view.width, view.height);
    scope.postMessage({ type: 'ready' });
    startLoop();
  } catch (error) {
    scope.postMessage({ type: 'failed', message: `${TEXT.rendererFailed}：${error instanceof Error ? error.message : String(error)}` });
  }
}

/** 接上画面端口：旧端口关闭，之后只收新端口的帧。 */
function attach(port: MessagePort): void {
  framePort?.close();
  framePort = port;
  port.addEventListener('message', (event) => {
    if (framePort === port) session.receive(event.data);
  });
  port.start();
}

/** 处理主线程发来的消息。 */
function handle(request: WorkerRequest): void {
  switch (request.type) {
    case 'init':
      initialize(request.canvas);
      return;
    case 'attach':
      attach(request.port);
      return;
    case 'view':
      // 页面隐藏时可能收到 0×0：保持上一次的画布尺寸，不把画布缩成 0。
      view = nextScreenView(view, request.view);
      renderer?.resize(view.width, view.height);
      // 改变画布尺寸会清空画布，等下一次 rAF 才重画会闪一下，所以立即重画。
      paint(undefined);
      return;
    case 'diagnostics': {
      if (request.reset) session.resetStatistics();
      const size = renderer?.size ?? { width: 0, height: 0 };
      scope.postMessage({ type: 'diagnostics', id: request.id, data: { ...session.diagnostics(), canvasWidth: size.width, canvasHeight: size.height, rendererReady: renderer !== undefined } });
      return;
    }
    case 'detach':
      framePort?.close();
      framePort = undefined;
      return;
  }
}

scope.addEventListener('message', (event) => {
  const request = parseWorkerRequest(event.data);
  if (request !== undefined) handle(request);
});
