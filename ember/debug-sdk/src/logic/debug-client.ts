/*
 * ------------------------------------------------------------------------
 * 名称：debug-client.ts
 * 说明：Ember 调试服务的客户端：建立连接并握手，发类型完整的请求，订阅与等待事件，按模块分组的接口（api）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：协议、认证与事件规则见 Ember 的 docs/04-调试服务.md；请求与事件的类型由 generated/api.ts 提供。
 * ------------------------------------------------------------------------
 */

import type { EmberApi, EventMap, RequestMap } from '../../generated/api.js';
import { PROTOCOL_VERSION } from '../../generated/api.js';
import type { CloseInfo, TransportFactory } from '../contracts/transport.js';
import { EmberConnectionError } from '../shared/errors.js';
import { DEFAULT_REQUEST_TIMEOUT_MS, RpcSession, type RequestOptions } from './rpc-session.js';

/** 握手结果：协议版本、核心与应用的版本、本连接的编号。 */
export type HelloInfo = RequestMap['debug.hello']['result'];

/** 连接的选项。 */
export interface ConnectOptions {
  /** 调试服务的地址，如 ws://127.0.0.1:5688/。 */
  readonly url: string;
  /** 令牌（首选项的“调试服务”分组里，或端点文件里）。 */
  readonly token: string;
  /** 客户端名称，显示在调试服务的连接列表里（1 到 64 个字符），默认 ember-debug-sdk。 */
  readonly clientName?: string;
  /** 请求默认的超时毫秒数，默认 60000；0 表示一直等。 */
  readonly requestTimeoutMs?: number;
  /** 建立连接与握手的超时毫秒数，默认 5000。 */
  readonly connectTimeoutMs?: number;
}

/** 带传输层的连接选项。 */
export interface OpenOptions extends ConnectOptions {
  /** 建立连接的工厂；默认的 WebSocket 实现见入口的 connect。 */
  readonly transportFactory: TransportFactory;
}

/** 等待事件的选项。 */
export interface WaitForOptions<E extends keyof EventMap> {
  /** 只接受满足条件的事件。 */
  readonly predicate?: (data: EventMap[E]) => boolean;
  /** 等待的毫秒数，默认 30000；0 表示一直等。 */
  readonly timeoutMs?: number;
}

/** 服务端接受的子协议名。 */
const SUBPROTOCOL = 'ember-debug.v1';
const DEFAULT_CLIENT_NAME = 'ember-debug-sdk';
const DEFAULT_CONNECT_TIMEOUT_MS = 5000;
const DEFAULT_WAIT_TIMEOUT_MS = 30_000;

/** kebab-case 的模块名（接口分组名是 camelCase）。 */
function toKebab(name: string): string {
  return name.replaceAll(/[A-Z]/g, (letter) => `-${letter.toLowerCase()}`);
}

/** 按模块分组的接口：api.runtime.run(...) 等同于 request('runtime.run', ...)。 */
function createApi(request: (method: string, params?: object, options?: RequestOptions) => Promise<unknown>): EmberApi {
  return new Proxy({} as EmberApi, {
    get: (_target, module) => {
      if (typeof module !== 'string' || module === 'then') return undefined;
      return new Proxy(
        {},
        {
          get: (_group, operation) => {
            if (typeof operation !== 'string' || operation === 'then') return undefined;
            return (params?: object) => request(`${toKebab(module)}.${operation}`, params);
          }
        }
      );
    }
  });
}

/** 连接到 Ember 调试服务的客户端。 */
export class EmberDebugClient {
  /** 按模块分组的接口，如 client.api.runtime.status()。 */
  readonly api: EmberApi;
  /** 连接关闭后兑现。 */
  readonly closed: Promise<CloseInfo>;
  private readonly eventHandlers = new Map<string, Set<(data: never) => void>>();
  private readonly anyEventHandlers = new Set<(event: string, data: unknown) => void>();

  private constructor(
    private readonly session: RpcSession,
    /** 握手结果。 */
    readonly info: HelloInfo
  ) {
    this.closed = session.closed;
    this.api = createApi((method, params, options) => session.request(method, params, options));
    session.onNotification((method, params) => this.dispatchEvent(method, params));
  }

  /**
   * 经给定的传输层工厂连接并握手（一般用入口的 connect）。
   * @throws EmberConnectionError 连不上、握手超时、协议不符。
   * @throws EmberRequestError 令牌不对（debug.unauthorized）。
   */
  static async open(options: OpenOptions): Promise<EmberDebugClient> {
    const factory = options.transportFactory;
    const connectTimeoutMs = options.connectTimeoutMs ?? DEFAULT_CONNECT_TIMEOUT_MS;
    let timer: ReturnType<typeof setTimeout> | undefined;
    const timeout = new Promise<never>((_resolve, reject) => {
      if (connectTimeoutMs > 0) timer = setTimeout(() => reject(new EmberConnectionError('timeout', `连接 ${options.url} 超时（${connectTimeoutMs} 毫秒）`)), connectTimeoutMs);
    });
    const attempt = (async () => {
      const transport = await factory(options.url, [SUBPROTOCOL]);
      const session = new RpcSession(transport, options.requestTimeoutMs ?? DEFAULT_REQUEST_TIMEOUT_MS);
      try {
        const info = (await session.request('debug.hello', { client: options.clientName ?? DEFAULT_CLIENT_NAME, token: options.token, protocol: PROTOCOL_VERSION }, { timeoutMs: connectTimeoutMs })) as HelloInfo;
        return new EmberDebugClient(session, info);
      } catch (error) {
        session.close();
        throw error;
      }
    })();
    try {
      return await Promise.race([attempt, timeout]);
    } catch (error) {
      // 超时后让还在进行的连接尝试自行收尾，不留未处理的拒绝。
      attempt.then((client) => client.close()).catch(() => undefined);
      throw error;
    } finally {
      if (timer !== undefined) clearTimeout(timer);
    }
  }

  /**
   * 发送请求并等结果。结果里的二进制 data（截图、内存）是 Uint8Array。
   * @throws EmberRequestError 服务端返回了错误，code 是模块里登记的错误码。
   * @throws EmberConnectionError 连接已关闭或等待超时。
   */
  request<M extends keyof RequestMap>(method: M, params?: RequestMap[M]['params'], options?: RequestOptions): Promise<RequestMap[M]['result']> {
    return this.session.request(method, params, options) as Promise<RequestMap[M]['result']>;
  }

  /** 订阅事件：事件名（如 runtime.runFinished）、模块名加 .*（一个模块的全部事件）或 *（全部）；返回订阅后本连接订阅的全部写法。 */
  async subscribe(events: readonly string[]): Promise<readonly string[]> {
    return (await this.request('debug.subscribe', { events: [...events] })).events;
  }

  /** 取消订阅。 */
  async unsubscribe(events: readonly string[]): Promise<readonly string[]> {
    return (await this.request('debug.unsubscribe', { events: [...events] })).events;
  }

  /** 注册某个事件的处理函数（要先 subscribe 才会收到）；返回取消注册的函数。 */
  on<E extends keyof EventMap>(event: E, handler: (data: EventMap[E]) => void): () => void {
    const handlers = this.eventHandlers.get(event) ?? new Set();
    handlers.add(handler as (data: never) => void);
    this.eventHandlers.set(event, handlers);
    return () => handlers.delete(handler as (data: never) => void);
  }

  /** 注册全部事件的处理函数（要先 subscribe 才会收到）；返回取消注册的函数。 */
  onAny(handler: (event: string, data: unknown) => void): () => void {
    this.anyEventHandlers.add(handler);
    return () => this.anyEventHandlers.delete(handler);
  }

  /**
   * 等下一个满足条件的事件（要先 subscribe）。
   * @throws EmberConnectionError 超时或连接关闭。
   */
  waitFor<E extends keyof EventMap>(event: E, options: WaitForOptions<E> = {}): Promise<EventMap[E]> {
    const timeoutMs = options.timeoutMs ?? DEFAULT_WAIT_TIMEOUT_MS;
    return new Promise<EventMap[E]>((resolve, reject) => {
      let timer: ReturnType<typeof setTimeout> | undefined;
      let settled = false;
      const finish = (action: () => void): void => {
        if (settled) return;
        settled = true;
        off();
        if (timer !== undefined) clearTimeout(timer);
        action();
      };
      const off = this.on(event, (data) => {
        if (options.predicate === undefined || options.predicate(data)) finish(() => resolve(data));
      });
      if (timeoutMs > 0) timer = setTimeout(() => finish(() => reject(new EmberConnectionError('timeout', `等待事件 ${event} 超时（${timeoutMs} 毫秒）`))), timeoutMs);
      void this.closed.then(() => finish(() => reject(new EmberConnectionError('closed', `等待事件 ${event} 时连接已关闭`))));
    });
  }

  /** 关闭连接。 */
  close(): void {
    this.session.close();
  }

  private dispatchEvent(event: string, data: unknown): void {
    for (const handler of this.eventHandlers.get(event) ?? []) (handler as (value: unknown) => void)(data);
    for (const handler of this.anyEventHandlers) handler(event, data);
  }
}
