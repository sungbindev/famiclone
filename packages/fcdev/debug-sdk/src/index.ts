/*
 * ------------------------------------------------------------------------
 * 名称：index.ts
 * 说明：调试 SDK 的入口（不依赖 Node 内置模块，浏览器与 Electron 页面也能用）：connect、EmberDebugClient、错误类型与全部请求、事件的类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：请求与事件的类型由 Ember 的 npm run schema 生成（generated/api.ts）；Node 环境下自动查找 Ember 的入口在 node.ts（构建后是 dist/debug/node.js）。
 * ------------------------------------------------------------------------
 */

export { connect, type ConnectWithTransportOptions } from './entry/connect.js';
export { EmberDebugClient, type ConnectOptions, type HelloInfo, type OpenOptions, type WaitForOptions } from './logic/debug-client.js';
export { decodeBinaryFrame } from './logic/frame-codec.js';
export type { RequestOptions } from './logic/rpc-session.js';
export type { CloseInfo, Transport, TransportFactory } from './contracts/transport.js';
export { EmberConnectionError, EmberDebugError, EmberRequestError, type ConnectionFailureKind } from './shared/errors.js';
export type { JsonRpcErrorBody, JsonRpcMessage, JsonRpcNotification, JsonRpcRequest, JsonRpcResponse, RequestId } from './shared/rpc-message.js';
export { PROTOCOL_VERSION } from '../generated/api.js';
export type * from '../generated/api.js';
