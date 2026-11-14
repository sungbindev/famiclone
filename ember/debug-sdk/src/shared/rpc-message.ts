/*
 * ------------------------------------------------------------------------
 * 名称：rpc-message.ts
 * 说明：JSON-RPC 2.0 报文的类型：请求、响应、通知与错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

/** 请求编号。 */
export type RequestId = number | string;

/** 请求。 */
export interface JsonRpcRequest {
  readonly jsonrpc: '2.0';
  readonly id: RequestId;
  readonly method: string;
  readonly params?: object;
}

/** 错误响应里的 error 字段。 */
export interface JsonRpcErrorBody {
  readonly code: number;
  readonly message: string;
  readonly data?: { readonly code?: string; readonly [key: string]: unknown };
}

/** 响应：result 与 error 二选一。 */
export interface JsonRpcResponse {
  readonly jsonrpc: '2.0';
  readonly id: RequestId | null;
  result?: unknown;
  readonly error?: JsonRpcErrorBody;
}

/** 通知（事件）：没有 id。 */
export interface JsonRpcNotification {
  readonly jsonrpc: '2.0';
  readonly method: string;
  params?: unknown;
}

/** 服务端发来的报文。 */
export type JsonRpcMessage = JsonRpcResponse | JsonRpcNotification;
