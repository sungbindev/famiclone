/*
 * ------------------------------------------------------------------------
 * 名称：worker-messages.ts
 * 说明：画面组件（主线程）与渲染 Worker 之间的消息格式，以及按消息类型校验的函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：Worker 只接受这里列出的消息；收到的消息先按类型与字段校验，格式不对的丢弃，不抛异常。
 *       帧端口的消息（帧与确认）不在这里，见 frame-receiver.ts 与 contracts/stream-ports。
 * ------------------------------------------------------------------------
 */

import type { ScreenAspect, ScreenScaling } from './screen-layout';
import type { RenderDiagnostics } from './render-stats';

/** 画布的版面：画布的设备像素尺寸、画面比例与缩放方式。 */
export interface ScreenView {
  readonly width: number;
  readonly height: number;
  readonly aspect: ScreenAspect;
  readonly scaling: ScreenScaling;
}

/** 非渲染统计的诊断：最近收到的一帧的像素摘要与画布尺寸。 */
export interface WorkerDiagnostics extends RenderDiagnostics {
  /** 最近收到的一帧的非黑像素比例；还没收到帧时为 -1。 */
  readonly lastFrameNonBlackRatio: number;
  /** 最近收到的一帧的像素哈希；还没收到帧时为 -1。 */
  readonly lastFrameHash: number;
  /** 最近收到的一帧的尺寸；还没收到帧时为 0。 */
  readonly frameWidth: number;
  readonly frameHeight: number;
  /** 画布当前的设备像素尺寸。 */
  readonly canvasWidth: number;
  readonly canvasHeight: number;
  /** 是否已经有可用的 WebGL 渲染器。 */
  readonly rendererReady: boolean;
}

/** 主线程发给 Worker 的消息。 */
export type WorkerRequest =
  | { readonly type: 'init'; readonly canvas: OffscreenCanvas }
  | { readonly type: 'attach'; readonly port: MessagePort }
  | { readonly type: 'view'; readonly view: ScreenView }
  | { readonly type: 'diagnostics'; readonly id: number; readonly reset: boolean }
  | { readonly type: 'detach' };

/** Worker 发给主线程的消息。 */
export type WorkerReply =
  | { readonly type: 'ready' }
  | { readonly type: 'failed'; readonly message: string }
  | { readonly type: 'diagnostics'; readonly id: number; readonly data: WorkerDiagnostics };

/** 画布尺寸的上限（设备像素）：防止异常的消息让 Worker 分配过大的画布。 */
export const MAX_CANVAS_DIMENSION = 16_384;

/** 是不是画布尺寸（0 到上限的整数）。 */
function isDimension(value: unknown): value is number {
  return typeof value === 'number' && Number.isInteger(value) && value >= 0 && value <= MAX_CANVAS_DIMENSION;
}

/** 校验版面。 */
function parseView(value: unknown): ScreenView | undefined {
  if (typeof value !== 'object' || value === null) return undefined;
  const { width, height, aspect, scaling } = value as Record<string, unknown>;
  if (!isDimension(width) || !isDimension(height)) return undefined;
  if (aspect !== '4:3' && aspect !== '8:7') return undefined;
  if (scaling !== 'fit' && scaling !== 'integer') return undefined;
  return { width, height, aspect, scaling };
}

/**
 * 校验主线程发给 Worker 的消息：类型与字段都要符合。
 * @param data 收到的数据。
 * @returns 合格的消息；不合格时为 undefined。
 */
export function parseWorkerRequest(data: unknown): WorkerRequest | undefined {
  if (typeof data !== 'object' || data === null) return undefined;
  const message = data as Record<string, unknown>;
  switch (message['type']) {
    case 'init':
      return typeof OffscreenCanvas !== 'undefined' && message['canvas'] instanceof OffscreenCanvas ? { type: 'init', canvas: message['canvas'] } : undefined;
    case 'attach':
      return typeof MessagePort !== 'undefined' && message['port'] instanceof MessagePort ? { type: 'attach', port: message['port'] } : undefined;
    case 'view': {
      const view = parseView(message['view']);
      return view === undefined ? undefined : { type: 'view', view };
    }
    case 'diagnostics': {
      const { id, reset } = message;
      return typeof id === 'number' && Number.isInteger(id) && id >= 0 && typeof reset === 'boolean' ? { type: 'diagnostics', id, reset } : undefined;
    }
    case 'detach':
      return { type: 'detach' };
    default:
      return undefined;
  }
}

/**
 * 校验 Worker 发给主线程的消息。
 * @param data 收到的数据。
 * @returns 合格的消息；不合格时为 undefined。
 */
export function parseWorkerReply(data: unknown): WorkerReply | undefined {
  if (typeof data !== 'object' || data === null) return undefined;
  const message = data as Record<string, unknown>;
  switch (message['type']) {
    case 'ready':
      return { type: 'ready' };
    case 'failed':
      return typeof message['message'] === 'string' ? { type: 'failed', message: message['message'] } : undefined;
    case 'diagnostics': {
      const { id, data: diagnostics } = message;
      return typeof id === 'number' && typeof diagnostics === 'object' && diagnostics !== null ? { type: 'diagnostics', id, data: diagnostics as WorkerDiagnostics } : undefined;
    }
    default:
      return undefined;
  }
}
