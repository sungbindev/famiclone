/*
 * ------------------------------------------------------------------------
 * 名称：connect.ts
 * 说明：SDK 的连接入口：connect（给定地址与令牌，默认用全局 WebSocket）与 connectToEmber（Node 环境，自动找到正在运行的 Ember）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：这里是组装根：把默认的传输层与端点发现注入客户端。
 * ------------------------------------------------------------------------
 */

import type { TransportFactory } from '../contracts/transport.js';
import { openWebSocket } from '../infra/web-socket-transport.js';
import { EmberDebugClient, type ConnectOptions } from '../logic/debug-client.js';

/** connect 的选项：在 ConnectOptions 之上可以换传输层。 */
export interface ConnectWithTransportOptions extends ConnectOptions {
  /** 自定义传输层；默认用全局的 WebSocket。 */
  readonly transportFactory?: TransportFactory;
}

/**
 * 连接到调试服务并握手。
 * @throws EmberConnectionError 连不上、握手超时。
 * @throws EmberRequestError 令牌不对（debug.unauthorized）。
 */
export function connect(options: ConnectWithTransportOptions): Promise<EmberDebugClient> {
  return EmberDebugClient.open({ ...options, transportFactory: options.transportFactory ?? openWebSocket });
}
