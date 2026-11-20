/*
 * ------------------------------------------------------------------------
 * 名称：rpc-session.ts
 * 说明：一条连接上的 JSON-RPC 会话：编号并发送请求，按编号匹配响应，转发通知，连接关闭时让等待中的请求失败。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import type { CloseInfo, Transport } from '../contracts/transport.js';
import { EmberConnectionError, EmberRequestError } from '../shared/errors.js';
import type { JsonRpcMessage, JsonRpcNotification, JsonRpcRequest, JsonRpcResponse } from '../shared/rpc-message.js';
import { decodeBinaryFrame } from './frame-codec.js';

/** 请求没有指定超时时使用的毫秒数。 */
export const DEFAULT_REQUEST_TIMEOUT_MS = 60_000;

/** 单个请求的选项。 */
export interface RequestOptions {
  /** 等待响应的毫秒数；0 表示一直等。超时后请求在服务端仍会执行完。 */
  readonly timeoutMs?: number;
}

/** 通知处理函数。 */
export type NotificationHandler = (method: string, params: unknown) => void;

interface PendingRequest {
  readonly method: string;
  readonly resolve: (result: unknown) => void;
  readonly reject: (error: Error) => void;
  timer?: ReturnType<typeof setTimeout>;
}

/** 一条连接上的 JSON-RPC 会话。 */
export class RpcSession {
  private nextId = 1;
  private readonly pending = new Map<number, PendingRequest>();
  private readonly notificationHandlers = new Set<NotificationHandler>();
  private closeInfo?: CloseInfo;
  /** 连接关闭后兑现。 */
  readonly closed: Promise<CloseInfo>;

  /**
   * @param transport 已打开的连接。
   * @param defaultTimeoutMs 请求默认的超时毫秒数。
   */
  constructor(
    private readonly transport: Transport,
    private readonly defaultTimeoutMs: number = DEFAULT_REQUEST_TIMEOUT_MS
  ) {
    this.closed = new Promise<CloseInfo>((resolve) => {
      transport.onClose((info) => {
        this.closeInfo = info;
        this.failAll(new EmberConnectionError('closed', `连接已关闭（${info.code}${info.reason === '' ? '' : `：${info.reason}`}）`, info.code));
        resolve(info);
      });
    });
    transport.onMessage((message) => this.receive(message));
  }

  /** 注册通知（事件）处理函数；返回取消注册的函数。 */
  onNotification(handler: NotificationHandler): () => void {
    this.notificationHandlers.add(handler);
    return () => this.notificationHandlers.delete(handler);
  }

  /**
   * 发送请求并等响应。
   * @throws EmberRequestError 服务端返回了错误。
   * @throws EmberConnectionError 连接已关闭或等待超时。
   */
  request(method: string, params?: object, options: RequestOptions = {}): Promise<unknown> {
    if (this.closeInfo !== undefined) return Promise.reject(new EmberConnectionError('closed', '连接已关闭', this.closeInfo.code));
    const id = this.nextId++;
    const message: JsonRpcRequest = { jsonrpc: '2.0', id, method, ...(params === undefined ? {} : { params }) };
    return new Promise<unknown>((resolve, reject) => {
      const entry: PendingRequest = { method, resolve, reject };
      const timeoutMs = options.timeoutMs ?? this.defaultTimeoutMs;
      if (timeoutMs > 0) {
        entry.timer = setTimeout(() => {
          this.pending.delete(id);
          reject(new EmberConnectionError('timeout', `等待 ${method} 的响应超时（${timeoutMs} 毫秒）`));
        }, timeoutMs);
      }
      this.pending.set(id, entry);
      try {
        this.transport.send(JSON.stringify(message));
      } catch (error) {
        this.settle(id)?.reject(new EmberConnectionError('closed', `无法发送 ${method}：${error instanceof Error ? error.message : String(error)}`));
      }
    });
  }

  /** 关闭连接。 */
  close(code = 1000, reason = ''): void {
    this.transport.close(code, reason);
  }

  private receive(raw: string | Uint8Array): void {
    let message: JsonRpcMessage;
    try {
      message = typeof raw === 'string' ? (JSON.parse(raw) as JsonRpcMessage) : decodeBinaryFrame(raw);
    } catch (error) {
      const failure = error instanceof EmberConnectionError ? error : new EmberConnectionError('protocol', '收到的报文不是合法的 JSON');
      this.failAll(failure);
      this.transport.close(1002, '报文不符合协议');
      return;
    }
    if ('method' in message) {
      this.dispatchNotification(message);
    } else {
      this.dispatchResponse(message);
    }
  }

  private dispatchNotification(message: JsonRpcNotification): void {
    for (const handler of this.notificationHandlers) handler(message.method, message.params);
  }

  private dispatchResponse(message: JsonRpcResponse): void {
    if (typeof message.id !== 'number') return;
    const entry = this.settle(message.id);
    if (entry === undefined) return;
    if (message.error !== undefined) {
      const { data, message: text, code } = message.error;
      entry.reject(new EmberRequestError(code, data?.code ?? 'core.unknown', text, data));
    } else {
      entry.resolve(message.result);
    }
  }

  /** 取出并清理一个等待中的请求。 */
  private settle(id: number): PendingRequest | undefined {
    const entry = this.pending.get(id);
    if (entry === undefined) return undefined;
    this.pending.delete(id);
    if (entry.timer !== undefined) clearTimeout(entry.timer);
    return entry;
  }

  private failAll(error: Error): void {
    for (const id of this.pending.keys()) this.settle(id)?.reject(error);
  }
}
