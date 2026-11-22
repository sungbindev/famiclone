// ------------------------------------------------------------------------
// 名称：debug-connector.ts
// 说明：连接 Ember 调试服务的能力：按地址与令牌连接，或自动找到正在运行的 Ember 再连接；以及接口描
//       述文档（OpenRPC）的来源。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：连接方式（WebSocket）与端点文件的位置由实现决定，使用者只经这两个接口。
// ------------------------------------------------------------------------

import type { OpenRpcDocument } from '../../shared/debug-client/method-catalog';
import type { ConnectOptions, DebugSession } from './debug-session';
import type { Endpoint, TargetOptions } from './endpoint';

/** 找到 Ember 再连接时的选项：地址与令牌可以省略，省略时自动查找。 */
export interface ConnectToEmberOptions extends TargetOptions, Partial<Pick<ConnectOptions, 'url' | 'token'>> {
  /** 客户端名称，显示在调试服务的连接列表里。 */
  readonly clientName?: string;
  /** 请求默认的超时毫秒数。 */
  readonly requestTimeoutMs?: number;
  /** 建立连接与握手的超时毫秒数。 */
  readonly connectTimeoutMs?: number;
}

/** 连接 Ember 调试服务。 */
export interface DebugConnector {
  /**
   * 按地址与令牌连接并握手。
   * @param options 连接选项。
   * @returns 调试会话；调用者负责关闭。
   * @throws EmberConnectionError 连不上、握手超时、协议版本不符。
   * @throws EmberRequestError 令牌不对（debug.unauthorized）。
   */
  connect(options: ConnectOptions): Promise<DebugSession>;

  /**
   * 找到正在运行的 Ember 并连接：地址与令牌先取显式参数，再取环境变量，缺的部分从端点文件读。
   * @param options 选项。
   * @returns 调试会话；调用者负责关闭。
   * @throws EmberConnectionError 找不到端点文件、连不上、握手超时。
   * @throws EmberRequestError 令牌不对。
   */
  connectToEmber(options?: ConnectToEmberOptions): Promise<DebugSession>;

  /**
   * 读出正在运行的 Ember 的端点文件。
   * @param options 显式参数。
   * @returns 端点文件的内容。
   * @throws EmberConnectionError 找不到端点文件、内容不对或进程已退出。
   */
  readEndpoint(options?: TargetOptions): Endpoint;
}

/** 调试协议接口描述文档的来源。 */
export interface OpenRpcDocumentSource {
  /**
   * 读出随扩展发布的接口描述文档：全部请求、事件与数据类型。
   * @returns 接口描述文档。
   */
  load(): OpenRpcDocument;
}
