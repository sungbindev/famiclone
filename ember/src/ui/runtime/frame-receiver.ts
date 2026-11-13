/*
 * ------------------------------------------------------------------------
 * 名称：frame-receiver.ts
 * 说明：渲染侧收到帧的处理：帧消息的解析与校验、“只保留最新一帧”的待画槽、帧像素的摘要（非黑像素比例与哈希）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯函数与纯类，在 Worker 里使用，不访问 DOM、Node 与 Electron。来自核心的帧数据属于外部输入，格式不符的帧直接丢弃并计数，不抛异常。
 *       消息格式见 src/contracts/stream-ports/stream-messages.ts（界面层只能引用 contracts 的类型，所以这里的常量与契约一致地写一份）。
 * ------------------------------------------------------------------------
 */

/** 画面的像素格式，与契约一致。 */
export const FRAME_FORMAT = 'rgba8';

/** 每像素的字节数。 */
export const BYTES_PER_PIXEL = 4;

/** 帧宽度与高度的上限：超过的当作格式错误，防止异常的消息让渲染端分配巨大的纹理。 */
export const MAX_FRAME_DIMENSION = 4096;

/** 解析并校验过的一帧。 */
export interface ParsedFrame {
  readonly width: number;
  readonly height: number;
  readonly frame: number;
  /** 核心宿主发出这一帧时的 Date.now()。 */
  readonly publishedAt: number;
  /** 核心宿主累计因背压丢弃的帧数。 */
  readonly hostDropped: number;
  readonly pixels: ArrayBuffer;
}

/** 帧的解析结果：失败时如果能读出帧号也一并给出，以便仍然确认这一帧，不占用核心的在途名额。 */
export type FrameParseResult = { readonly ok: true; readonly value: ParsedFrame } | { readonly ok: false; readonly frame: number | undefined };

/** 是不是非负整数。 */
function isCount(value: unknown): value is number {
  return typeof value === 'number' && Number.isInteger(value) && value >= 0;
}

/** 是不是 ArrayBuffer（跨 Worker 与页面的实现不用 instanceof）。 */
function isArrayBuffer(value: unknown): value is ArrayBuffer {
  return Object.prototype.toString.call(value) === '[object ArrayBuffer]';
}

/**
 * 解析并校验画面端口上收到的一条消息：类型、尺寸、像素格式、字节数都要符合。
 * @param data 收到的数据。
 * @returns 合格的帧；不合格时 ok 为 false，能读出帧号时带着帧号。
 */
export function parseFrameMessage(data: unknown): FrameParseResult {
  if (typeof data !== 'object' || data === null) return { ok: false, frame: undefined };
  const { type, width, height, format, frame, publishedAt, hostDropped, pixels } = data as Record<string, unknown>;
  const knownFrame = isCount(frame) ? frame : undefined;
  if (type !== 'frame' || knownFrame === undefined) return { ok: false, frame: undefined };
  if (!isCount(width) || !isCount(height) || width < 1 || height < 1 || width > MAX_FRAME_DIMENSION || height > MAX_FRAME_DIMENSION) return { ok: false, frame: knownFrame };
  if (format !== FRAME_FORMAT) return { ok: false, frame: knownFrame };
  if (typeof publishedAt !== 'number' || !Number.isFinite(publishedAt) || !isCount(hostDropped)) return { ok: false, frame: knownFrame };
  if (!isArrayBuffer(pixels) || pixels.byteLength !== width * height * BYTES_PER_PIXEL) return { ok: false, frame: knownFrame };
  return { ok: true, value: { width, height, frame: knownFrame, publishedAt, hostDropped, pixels } };
}

/** 只保留最新一帧的待画槽：新帧到达时顶掉还没画的旧帧。 */
export class LatestFrameSlot<T> {
  private pending: T | undefined;

  /**
   * 放入新帧。
   * @returns 被顶掉的、还没画出的旧帧；没有时为 undefined。
   */
  offer(item: T): T | undefined {
    const replaced = this.pending;
    this.pending = item;
    return replaced;
  }

  /** 取走待画的帧；没有时为 undefined。 */
  take(): T | undefined {
    const item = this.pending;
    this.pending = undefined;
    return item;
  }
}

/** 帧像素的摘要。 */
export interface PixelSummary {
  /** 颜色不全为 0 的像素占全部像素的比例，0 到 1。 */
  readonly nonBlackRatio: number;
  /** 全部像素的 FNV-1a 哈希（32 位无符号整数），用来判断两帧是否相同。 */
  readonly hash: number;
}

/**
 * 统计一帧像素：非黑像素的比例与哈希；颜色只看 R、G、B，不看 A。
 * @param pixels RGBA8 像素，字节数须是 4 的倍数。
 */
export function summarizePixels(pixels: ArrayBuffer): PixelSummary {
  const words = new Uint32Array(pixels, 0, Math.floor(pixels.byteLength / BYTES_PER_PIXEL));
  let nonBlack = 0;
  let hash = 2166136261;
  for (const word of words) {
    if ((word & 0x00ffffff) !== 0) nonBlack += 1;
    hash = Math.imul(hash ^ word, 16777619) >>> 0;
  }
  return { nonBlackRatio: words.length === 0 ? 0 : nonBlack / words.length, hash };
}
