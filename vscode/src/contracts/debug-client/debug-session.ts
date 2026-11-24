// ------------------------------------------------------------------------
// 名称：debug-session.ts
// 说明：到 Ember 调试服务的一条连接（调试会话）的接口，以及连接、请求与等待事件的选项。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：请求与事件的类型由 Ember 的 schema 生成（generated/api.ts）；模拟器操作与调试命令只依赖这个
//       接口，不关心连接怎么建立。
// ------------------------------------------------------------------------

import type { EmberApi, EventMap, RequestMap } from './generated/api';
import type { CloseInfo } from './transport';

/** 握手结果：协议版本、核心与应用的版本、本连接的编号。 */
export type HelloInfo = RequestMap['debug.hello']['result'];

/** 连接的选项。 */
export interface ConnectOptions {
  /** 调试服务的地址，如 ws://127.0.0.1:5688/。 */
  readonly url: string;
  /** 令牌（首选项的“调试服务”分组里，或端点文件里）。 */
  readonly token: string;
  /** 客户端名称，显示在调试服务的连接列表里（1 到 64 个字符），默认 FC Dev。 */
  readonly clientName?: string;
  /** 请求默认的超时毫秒数，默认 60000；0 表示一直等。 */
  readonly requestTimeoutMs?: number;
  /** 建立连接与握手的超时毫秒数，默认 5000。 */
  readonly connectTimeoutMs?: number;
}

/** 单个请求的选项。 */
export interface RequestOptions {
  /** 等待响应的毫秒数；0 表示一直等。超时后请求在服务端仍会执行完。 */
  readonly timeoutMs?: number;
}

/** 等待事件的选项。 */
export interface WaitForOptions<E extends keyof EventMap> {
  /** 只接受满足条件的事件。 */
  readonly predicate?: (data: EventMap[E]) => boolean;
  /** 等待的毫秒数，默认 30000；0 表示一直等。 */
  readonly timeoutMs?: number;
}

/** 到 Ember 调试服务的一条连接。 */
export interface DebugSession {
  /** 按模块分组的接口，如 session.api.runtime.status()。 */
  readonly api: EmberApi;
  /** 握手结果。 */
  readonly info: HelloInfo;
  /** 连接关闭后兑现。 */
  readonly closed: Promise<CloseInfo>;

  /**
   * 发送请求并等结果。结果里的二进制 data（截图、内存）是 Uint8Array。
   * @param method 请求名。
   * @param params 请求参数。
   * @param options 超时等选项。
   * @returns 请求的结果。
   * @throws EmberRequestError 服务端返回了错误，code 是模块里登记的错误码。
   * @throws EmberConnectionError 连接已关闭或等待超时。
   */
  request<M extends keyof RequestMap>(
    method: M,
    params?: RequestMap[M]['params'],
    options?: RequestOptions
  ): Promise<RequestMap[M]['result']>;

  /**
   * 订阅事件：事件名（如 runtime.runFinished）、模块名加 .*（一个模块的全部事件）或 *（全部）。
   * @param events 要订阅的事件写法。
   * @returns 订阅后本连接订阅的全部写法。
   */
  subscribe(events: readonly string[]): Promise<readonly string[]>;

  /**
   * 取消订阅。
   * @param events 要取消的事件写法。
   * @returns 取消后本连接订阅的全部写法。
   */
  unsubscribe(events: readonly string[]): Promise<readonly string[]>;

  /**
   * 注册某个事件的处理函数（要先 subscribe 才会收到）。
   * @param event 事件名。
   * @param handler 处理函数。
   * @returns 取消注册的函数。
   */
  on<E extends keyof EventMap>(event: E, handler: (data: EventMap[E]) => void): () => void;

  /**
   * 注册全部事件的处理函数（要先 subscribe 才会收到）。
   * @param handler 处理函数。
   * @returns 取消注册的函数。
   */
  onAny(handler: (event: string, data: unknown) => void): () => void;

  /**
   * 等下一个满足条件的事件（要先 subscribe）。
   * @param event 事件名。
   * @param options 条件与超时。
   * @returns 事件的数据。
   * @throws EmberConnectionError 超时或连接关闭。
   */
  waitFor<E extends keyof EventMap>(event: E, options?: WaitForOptions<E>): Promise<EventMap[E]>;

  /** 关闭连接。 */
  close(): void;
}
