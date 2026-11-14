/*
 * ------------------------------------------------------------------------
 * 名称：core-gateway.ts
 * 说明：核心网关接口：主进程经它向核心发送请求、订阅事件与运行状态；实现经消息端口调用本机核心进程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：实现只在组装根创建并注入（见 docs/01-软件架构设计.md 第 2 节）。
 * ------------------------------------------------------------------------
 */

import type { CoreStatus } from './core-messages';
import type { CoreEventName, CoreParams, CoreRequestName, CoreResult } from './generated/core-api';

/** 请求选项。 */
export interface CoreRequestOptions {
  /** 中止时向核心发送取消请求，Promise 以 core.cancelled 拒绝。 */
  readonly signal?: AbortSignal;
}

/** 核心网关。 */
export interface CoreGateway {
  /** 发送请求；失败时以 CoreRequestError 拒绝。 */
  request<N extends CoreRequestName>(name: N, params: CoreParams<N>, options?: CoreRequestOptions): Promise<CoreResult<N>>;

  /** 订阅核心事件；返回取消订阅的函数。 */
  onEvent(listener: (name: CoreEventName, data: unknown) => void): () => void;

  /** 订阅核心运行状态的变化（启动中、就绪、不可用，核心进程重启也会依次变化）；返回取消订阅的函数。 */
  onStatusChange(listener: (status: CoreStatus) => void): () => void;

  /** 当前的运行状态。 */
  status(): CoreStatus;
}
