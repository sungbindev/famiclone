/*
 * ------------------------------------------------------------------------
 * 名称：errors.ts
 * 说明：SDK 的错误类型：连接类错误（EmberConnectionError）与服务端返回的请求错误（EmberRequestError）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

/** 连接类错误的种类。 */
export type ConnectionFailureKind =
  /** 无法连上调试服务（没有运行、没有开启、端口不对）。 */
  | 'unavailable'
  /** 连接已关闭。 */
  | 'closed'
  /** 等待连接或响应超时。 */
  | 'timeout'
  /** 对方发来的报文不符合协议。 */
  | 'protocol'
  /** 找不到端点文件，或端点文件已过期。 */
  | 'endpoint';

/** 所有 SDK 错误的基类。 */
export class EmberDebugError extends Error {
  override name = 'EmberDebugError';
}

/** 连接类错误：连不上、被关闭、超时、协议不符。 */
export class EmberConnectionError extends EmberDebugError {
  override name = 'EmberConnectionError';

  /**
   * @param kind 错误种类。
   * @param message 说明（中文）。
   * @param closeCode 连接被关闭时的关闭码（见调试服务文档 2.1 节）。
   */
  constructor(
    readonly kind: ConnectionFailureKind,
    message: string,
    readonly closeCode?: number
  ) {
    super(message);
  }
}

/** 服务端返回的错误响应。 */
export class EmberRequestError extends EmberDebugError {
  override name = 'EmberRequestError';

  /**
   * @param rpcCode JSON-RPC 错误码（-32602 参数不合格、-32000 业务错误等）。
   * @param code 模块里登记的错误码，如 runtime.noCartridge；没有时是 core.unknown。
   * @param message 服务端给的说明。
   * @param details 错误响应里 data 的其余内容。
   */
  constructor(
    readonly rpcCode: number,
    readonly code: string,
    message: string,
    readonly details?: unknown
  ) {
    super(message);
  }
}
