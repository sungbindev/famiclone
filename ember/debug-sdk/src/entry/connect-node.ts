/*
 * ------------------------------------------------------------------------
 * 名称：connect-node.ts
 * 说明：Node 环境的连接入口：connectToEmber 按显式参数、环境变量、端点文件的顺序找到正在运行的 Ember 并连接。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { resolveTarget, type TargetOptions } from '../infra/endpoint-file.js';
import type { EmberDebugClient } from '../logic/debug-client.js';
import { connect, type ConnectWithTransportOptions } from './connect.js';

/** connectToEmber 的选项：连接选项里的 url 与 token 可以省略，省略时自动查找。 */
export interface ConnectToEmberOptions extends TargetOptions, Omit<ConnectWithTransportOptions, 'url' | 'token'> {}

/**
 * 找到正在运行的 Ember 并连接：地址与令牌先取显式参数，再取环境变量
 * （EMBER_DEBUG_URL、EMBER_DEBUG_TOKEN），缺的部分从端点文件读。
 * @throws EmberConnectionError 找不到端点文件、连不上、握手超时。
 * @throws EmberRequestError 令牌不对。
 */
export function connectToEmber(options: ConnectToEmberOptions = {}): Promise<EmberDebugClient> {
  const { url, token } = resolveTarget(options);
  return connect({ ...options, url, token });
}
