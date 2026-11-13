/*
 * ------------------------------------------------------------------------
 * 名称：diagnostics-client.ts
 * 说明：主线程一侧的诊断请求器：向渲染 Worker 发诊断请求，按编号配对回复，超时或关闭时拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只在开发期由画面组件挂到 window 上，供 tools/dev 里的脚本经 CDP 读取；正式包不暴露。
 * ------------------------------------------------------------------------
 */

import type { WorkerDiagnostics } from './worker-messages';

/** 等待回复的最长时间（毫秒）。 */
const REPLY_TIMEOUT_MS = 5000;

const TEXT = {
  timeout: '渲染 Worker 没有及时回复诊断',
  closed: '渲染 Worker 已关闭'
} as const;

/** 一个等待中的请求。 */
interface Waiting {
  readonly resolve: (data: WorkerDiagnostics) => void;
  readonly reject: (error: Error) => void;
  readonly timer: ReturnType<typeof setTimeout>;
}

/** 诊断请求器。 */
export class DiagnosticsClient {
  private nextId = 0;
  private readonly waiting = new Map<number, Waiting>();

  /** @param post 把请求发给 Worker。 */
  constructor(private readonly post: (request: { readonly type: 'diagnostics'; readonly id: number; readonly reset: boolean }) => void) {}

  /**
   * 读取诊断。
   * @param reset 为 true 时先清空统计再读（用来开始一轮测量）。
   */
  read(reset = false): Promise<WorkerDiagnostics> {
    const id = this.nextId;
    this.nextId += 1;
    return new Promise<WorkerDiagnostics>((resolve, reject) => {
      const timer = setTimeout(() => {
        this.waiting.delete(id);
        reject(new Error(TEXT.timeout));
      }, REPLY_TIMEOUT_MS);
      this.waiting.set(id, { resolve, reject, timer });
      this.post({ type: 'diagnostics', id, reset });
    });
  }

  /** 收到 Worker 的诊断回复：按编号交给等待的请求，没有人等的回复忽略。 */
  handleReply(id: number, data: WorkerDiagnostics): void {
    const entry = this.waiting.get(id);
    if (entry === undefined) return;
    this.waiting.delete(id);
    clearTimeout(entry.timer);
    entry.resolve(data);
  }

  /** 关闭：拒绝全部等待中的请求。 */
  dispose(): void {
    for (const entry of this.waiting.values()) {
      clearTimeout(entry.timer);
      entry.reject(new Error(TEXT.closed));
    }
    this.waiting.clear();
  }
}
