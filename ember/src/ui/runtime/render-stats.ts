/*
 * ------------------------------------------------------------------------
 * 名称：render-stats.ts
 * 说明：渲染端的统计：收到、渲染、丢弃与无效的帧数，核心帧率、收到帧率、渲染帧率，以及核心发布到收到、收到到画出的延迟（平均、p95、最大）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯类，时钟由调用方传入。收到、画出的间隔用 Worker 的单调时钟（performance.now，毫秒）；核心发布到收到要跨进程，
 *       用两边同一台机器的 Date.now()（毫秒，分辨率 1 毫秒）。“画出”指绘制命令提交完成，不含合成器与显示器的延迟。
 * ------------------------------------------------------------------------
 */

/** 延迟样本的上限，超过后丢弃最早的样本：约 10 分钟的帧。 */
const MAX_SAMPLES = 40_000;

/** 一组延迟的摘要（毫秒）。 */
export interface LatencySummary {
  readonly count: number;
  readonly average: number;
  readonly p95: number;
  readonly max: number;
  readonly min: number;
}

/** 渲染端的统计结果。 */
export interface RenderDiagnostics {
  /** 收到并通过校验的帧数。 */
  readonly received: number;
  /** 已画出的帧数。 */
  readonly rendered: number;
  /** 收到但没来得及画就被更新的帧顶掉的帧数。 */
  readonly dropped: number;
  /** 格式不符被丢弃的帧数。 */
  readonly invalid: number;
  /** 帧号出现缺口、说明核心一侧没有送到的帧数（核心宿主的背压丢帧与 binding 没有空槽的丢帧都在其中）。 */
  readonly coreMissed: number;
  /** 核心宿主因背压丢弃的帧数（取自帧消息里的累计值，只算统计开始之后的）。 */
  readonly hostDropped: number;
  /** 最近收到的帧号。 */
  readonly lastFrame: number;
  /** 统计的时间跨度（毫秒）：第一帧到最后一帧的收到时间。 */
  readonly spanMs: number;
  /** 核心帧率：帧号的增量除以发布时间的增量（帧每秒）。 */
  readonly coreFps: number;
  /** 收到帧率（帧每秒）。 */
  readonly receivedFps: number;
  /** 渲染帧率（帧每秒）。 */
  readonly renderedFps: number;
  /** requestAnimationFrame 的触发率（次每秒），近似显示器的刷新率，用来解释渲染帧率与丢帧。 */
  readonly tickFps: number;
  /** 核心发布到渲染端收到的延迟。 */
  readonly publishToReceive: LatencySummary;
  /** 收到到画出的延迟。 */
  readonly receiveToRender: LatencySummary;
}

/** 空的延迟摘要。 */
const EMPTY_LATENCY: LatencySummary = { count: 0, average: 0, p95: 0, max: 0, min: 0 };

/** 把样本汇总成平均、p95（最近秩法）、最大与最小。 */
export function summarizeLatency(samples: readonly number[]): LatencySummary {
  if (samples.length === 0) return EMPTY_LATENCY;
  const sorted = samples.toSorted((a, b) => a - b);
  const total = sorted.reduce((sum, value) => sum + value, 0);
  const rank = Math.max(1, Math.ceil(sorted.length * 0.95));
  return { count: sorted.length, average: total / sorted.length, p95: sorted[rank - 1] ?? 0, max: sorted[sorted.length - 1] ?? 0, min: sorted[0] ?? 0 };
}

/** 每秒的速率：count 个事件之间有 count - 1 个间隔。 */
function ratePerSecond(count: number, spanMs: number): number {
  return count > 1 && spanMs > 0 ? ((count - 1) * 1000) / spanMs : 0;
}

/** 渲染端的统计。 */
export class RenderStatistics {
  private received = 0;
  private rendered = 0;
  private dropped = 0;
  private invalid = 0;
  private coreMissed = 0;
  private lastFrame = -1;
  private lastHostDropped = 0;
  private hostDroppedBase: number | undefined;
  private coreFrames = 0;
  private coreMs = 0;
  private lastPublishedAt = 0;
  private firstReceivedAt: number | undefined;
  private lastReceivedAt = 0;
  private firstRenderedAt: number | undefined;
  private lastRenderedAt = 0;
  private ticks = 0;
  private firstTickAt: number | undefined;
  private lastTickAt = 0;
  private publishToReceive: number[] = [];
  private receiveToRender: number[] = [];

  /**
   * 记录收到一帧。
   * @param frame 帧号。
   * @param publishedAt 核心发布时间（Date.now()）。
   * @param hostDropped 核心宿主累计因背压丢弃的帧数。
   * @param now 单调时钟的当前时间（毫秒）。
   * @param wallNow 墙上时钟的当前时间（Date.now()）。
   */
  onReceived(frame: number, publishedAt: number, hostDropped: number, now: number, wallNow: number): void {
    this.received += 1;
    this.firstReceivedAt ??= now;
    this.lastReceivedAt = now;
    this.hostDroppedBase ??= hostDropped;
    this.lastHostDropped = hostDropped;
    if (this.lastFrame >= 0 && frame > this.lastFrame) {
      this.coreMissed += frame - this.lastFrame - 1;
      if (publishedAt >= this.lastPublishedAt) {
        this.coreFrames += frame - this.lastFrame;
        this.coreMs += publishedAt - this.lastPublishedAt;
      }
    }
    this.lastFrame = frame;
    this.lastPublishedAt = publishedAt;
    this.push(this.publishToReceive, wallNow - publishedAt);
  }

  /** 记录一帧被更新的帧顶掉，没有画出。 */
  onDropped(): void {
    this.dropped += 1;
  }

  /** 记录一帧格式不符。 */
  onInvalid(): void {
    this.invalid += 1;
  }

  /**
   * 记录画出一帧。
   * @param receivedAt 这一帧收到时的单调时钟时间。
   * @param renderedAt 画完时的单调时钟时间。
   */
  onRendered(receivedAt: number, renderedAt: number): void {
    this.rendered += 1;
    this.firstRenderedAt ??= renderedAt;
    this.lastRenderedAt = renderedAt;
    this.push(this.receiveToRender, renderedAt - receivedAt);
  }

  /** 记录浏览器要画下一帧的节拍（requestAnimationFrame 触发一次）。 */
  onTick(now: number): void {
    this.ticks += 1;
    this.firstTickAt ??= now;
    this.lastTickAt = now;
  }

  /** 清空，重新开始统计。 */
  reset(): void {
    this.ticks = 0;
    this.firstTickAt = undefined;
    this.received = 0;
    this.rendered = 0;
    this.dropped = 0;
    this.invalid = 0;
    this.coreMissed = 0;
    this.lastFrame = -1;
    this.hostDroppedBase = undefined;
    this.coreFrames = 0;
    this.coreMs = 0;
    this.firstReceivedAt = undefined;
    this.firstRenderedAt = undefined;
    this.publishToReceive = [];
    this.receiveToRender = [];
  }

  /** 当前的统计结果。 */
  snapshot(): RenderDiagnostics {
    const receiveSpan = this.firstReceivedAt === undefined ? 0 : this.lastReceivedAt - this.firstReceivedAt;
    const renderSpan = this.firstRenderedAt === undefined ? 0 : this.lastRenderedAt - this.firstRenderedAt;
    return {
      received: this.received,
      rendered: this.rendered,
      dropped: this.dropped,
      invalid: this.invalid,
      coreMissed: this.coreMissed,
      hostDropped: Math.max(0, this.lastHostDropped - (this.hostDroppedBase ?? this.lastHostDropped)),
      lastFrame: this.lastFrame,
      spanMs: receiveSpan,
      coreFps: this.coreMs > 0 ? (this.coreFrames * 1000) / this.coreMs : 0,
      receivedFps: ratePerSecond(this.received, receiveSpan),
      renderedFps: ratePerSecond(this.rendered, renderSpan),
      tickFps: ratePerSecond(this.ticks, this.firstTickAt === undefined ? 0 : this.lastTickAt - this.firstTickAt),
      publishToReceive: summarizeLatency(this.publishToReceive),
      receiveToRender: summarizeLatency(this.receiveToRender)
    };
  }

  /** 追加样本，超过上限时丢弃最早的。 */
  private push(samples: number[], value: number): void {
    samples.push(value);
    if (samples.length > MAX_SAMPLES) samples.shift();
  }
}
