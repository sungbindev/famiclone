/*
 * ------------------------------------------------------------------------
 * 名称：frame-session.ts
 * 说明：渲染 Worker 的帧接收会话：校验收到的帧、只保留最新一帧、被顶掉或无效的帧立即确认、画出后确认、统计与诊断。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：不依赖任何 Worker 或 DOM 对象，时钟与确认函数由调用方传入，便于单元测试。
 *       确认的约定（与核心宿主的背压配合，见 contracts/stream-ports）：每一帧最终都恰好确认一次——画出后确认、
 *       被更新的帧顶掉时确认、格式不符但能读出帧号时确认，这样核心宿主的在途名额不会被占住。
 * ------------------------------------------------------------------------
 */

import { LatestFrameSlot, parseFrameMessage, summarizePixels, type ParsedFrame } from './frame-receiver';
import { RenderStatistics } from './render-stats';
import type { WorkerDiagnostics } from './worker-messages';

/** 会话需要的时钟。 */
export interface SessionClock {
  /** 单调时钟（毫秒），用来量收到与画出之间的间隔。 */
  now(): number;
  /** 墙上时钟（Date.now()），用来与核心发布时间相减。 */
  wallNow(): number;
}

/** 等着画的一帧：帧与收到它的时间。 */
export interface PendingFrame {
  readonly frame: ParsedFrame;
  /** 收到时的单调时钟（毫秒）。 */
  readonly receivedAt: number;
}

/** 会话给出的诊断：不含画布与渲染器的状态，那些由 Worker 补充。 */
export type SessionDiagnostics = Omit<WorkerDiagnostics, 'canvasWidth' | 'canvasHeight' | 'rendererReady'>;

/** 帧接收会话。 */
export class FrameSession {
  private readonly slot = new LatestFrameSlot<PendingFrame>();
  private lastReceived: ParsedFrame | undefined;
  readonly statistics = new RenderStatistics();

  /**
   * @param acknowledge 确认一帧：把帧号回给核心。
   * @param clock 时钟。
   */
  constructor(
    private readonly acknowledge: (frame: number) => void,
    private readonly clock: SessionClock
  ) {}

  /**
   * 处理画面端口上收到的一条消息。
   * 格式不符的帧计数后丢弃（能读出帧号时确认）；合格的帧放进待画槽，顶掉的旧帧计数并确认。
   */
  receive(data: unknown): void {
    const result = parseFrameMessage(data);
    if (!result.ok) {
      this.statistics.onInvalid();
      if (result.frame !== undefined) this.acknowledge(result.frame);
      return;
    }
    const now = this.clock.now();
    const { frame } = result.value;
    this.statistics.onReceived(frame, result.value.publishedAt, result.value.hostDropped, now, this.clock.wallNow());
    this.lastReceived = result.value;
    const replaced = this.slot.offer({ frame: result.value, receivedAt: now });
    if (replaced !== undefined) {
      this.statistics.onDropped();
      this.acknowledge(replaced.frame.frame);
    }
  }

  /** 取走待画的帧；没有新帧时为 undefined。 */
  takePending(): PendingFrame | undefined {
    return this.slot.take();
  }

  /** 一帧画完：记录统计并确认。 */
  completeRender(pending: PendingFrame): void {
    this.statistics.onRendered(pending.receivedAt, this.clock.now());
    this.acknowledge(pending.frame.frame);
  }

  /** 清空统计，重新开始。 */
  resetStatistics(): void {
    this.statistics.reset();
  }

  /** 当前的诊断：统计结果加最近收到的一帧的像素摘要。 */
  diagnostics(): SessionDiagnostics {
    const last = this.lastReceived;
    const summary = last === undefined ? undefined : summarizePixels(last.pixels);
    return {
      ...this.statistics.snapshot(),
      lastFrameNonBlackRatio: summary?.nonBlackRatio ?? -1,
      lastFrameHash: summary?.hash ?? -1,
      frameWidth: last?.width ?? 0,
      frameHeight: last?.height ?? 0
    };
  }
}
