/*
 * ------------------------------------------------------------------------
 * 名称：backend.ts
 * 说明：MCP 服务的后端：按需连接 Ember（连接断了下次调用自动重连），发请求，记录订阅的事件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import type { RequestMap } from '../../generated/api.js';
import type { EmberDebugClient } from '../logic/debug-client.js';
import type { McpBackend } from './server.js';

/** 事件记录最多保留的条数，超过时丢最早的。 */
export const MAX_RECORDED_EVENTS = 1000;

/** 记录的一条事件。 */
interface RecordedEvent {
  readonly event: string;
  readonly data: unknown;
}

/** 连接 Ember 的后端。 */
export class EmberBackend implements McpBackend {
  private client?: EmberDebugClient;
  private connecting?: Promise<EmberDebugClient>;
  private readonly patterns = new Set<string>();
  private recorded: RecordedEvent[] = [];

  /**
   * @param connect 建立一条已握手的连接。
   * @param requestTimeoutMs 每个请求等响应的最长毫秒数。
   */
  constructor(
    private readonly connect: () => Promise<EmberDebugClient>,
    private readonly requestTimeoutMs: number
  ) {}

  async call(method: string, params: object): Promise<unknown> {
    const client = await this.ensureClient();
    return client.request(method as keyof RequestMap, params as never, { timeoutMs: this.requestTimeoutMs });
  }

  async drainEvents(patterns: readonly string[] | undefined, max: number): Promise<{ events: ReadonlyArray<{ event: string; data: unknown }>; remaining: number }> {
    const fresh = (patterns ?? (this.patterns.size === 0 ? ['*'] : [])).filter((pattern) => !this.patterns.has(pattern));
    for (const pattern of fresh) this.patterns.add(pattern);
    const client = await this.ensureClient();
    if (fresh.length > 0) await client.subscribe(fresh);
    const events = this.recorded.slice(0, max);
    this.recorded = this.recorded.slice(max);
    return { events, remaining: this.recorded.length };
  }

  /** 取得可用的连接：没有或已断开时重新连接，并重新订阅记录中的事件。 */
  private async ensureClient(): Promise<EmberDebugClient> {
    if (this.client !== undefined) return this.client;
    this.connecting ??= this.open().finally(() => {
      this.connecting = undefined;
    });
    return this.connecting;
  }

  private async open(): Promise<EmberDebugClient> {
    const client = await this.connect();
    client.onAny((event, data) => {
      this.recorded.push({ event, data });
      if (this.recorded.length > MAX_RECORDED_EVENTS) this.recorded.splice(0, this.recorded.length - MAX_RECORDED_EVENTS);
    });
    void client.closed.then(() => {
      if (this.client === client) this.client = undefined;
    });
    if (this.patterns.size > 0) await client.subscribe([...this.patterns]);
    this.client = client;
    return client;
  }
}
