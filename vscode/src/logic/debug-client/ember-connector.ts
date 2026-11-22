// ------------------------------------------------------------------------
// 名称：ember-connector.ts
// 说明：连接 Ember 调试服务的实现：把传输层与端点查找接在 EmberDebugClient 上，对外给出
//       DebugConnector。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：传输层（WebSocket）与端点查找（端点文件、环境变量）都由外部注入，测试时可以换成假的。
// ------------------------------------------------------------------------

import type { ConnectToEmberOptions, DebugConnector } from '../../contracts/debug-client/debug-connector';
import type { ConnectOptions, DebugSession } from '../../contracts/debug-client/debug-session';
import type { Endpoint, EndpointResolver, TargetOptions } from '../../contracts/debug-client/endpoint';
import type { TransportFactory } from '../../contracts/debug-client/transport';
import { EmberDebugClient } from './ember-debug-client';

/** 经注入的传输层与端点查找连接 Ember。 */
export class EmberConnector implements DebugConnector {
  /**
   * @param transportFactory 建立连接的工厂。
   * @param endpoints 查找正在运行的 Ember 的调试服务。
   */
  constructor(
    private readonly transportFactory: TransportFactory,
    private readonly endpoints: EndpointResolver
  ) {}

  /** @inheritdoc */
  connect(options: ConnectOptions): Promise<DebugSession> {
    return EmberDebugClient.open({ ...options, transportFactory: this.transportFactory });
  }

  /** @inheritdoc */
  connectToEmber(options: ConnectToEmberOptions = {}): Promise<DebugSession> {
    const { url, token } = this.endpoints.resolveTarget(options);
    const { clientName, requestTimeoutMs, connectTimeoutMs } = options;
    return this.connect({
      url,
      token,
      ...(clientName === undefined ? {} : { clientName }),
      ...(requestTimeoutMs === undefined ? {} : { requestTimeoutMs }),
      ...(connectTimeoutMs === undefined ? {} : { connectTimeoutMs })
    });
  }

  /** @inheritdoc */
  readEndpoint(options: TargetOptions = {}): Endpoint {
    return this.endpoints.readEndpoint(options);
  }
}
