/*
 * ------------------------------------------------------------------------
 * 名称：core-messages.ts
 * 说明：主进程与核心进程之间的消息、核心的通用错误码与运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：错误码与 core/src/shared/errors/core-error.hpp 一致；参数与结果是普通 JSON 值，核心进程负责与原生模块之间的 JSON 文本转换。
 * ------------------------------------------------------------------------
 */

/** 核心的通用错误码；各模块的错误码在模块 Schema 中登记。 */
export const CORE_ERROR_CODES = {
  invalidParams: 'core.invalidParams',
  unknownRequest: 'core.unknownRequest',
  cancelled: 'core.cancelled',
  unavailable: 'core.unavailable',
  internal: 'core.internal'
} as const;

/** 错误：错误码与面向用户的说明。 */
export interface CoreErrorInfo {
  readonly code: string;
  readonly message: string;
}

/** 核心的运行状态。 */
export type CoreStatus =
  | { readonly state: 'starting' }
  | { readonly state: 'ready'; readonly coreVersion: string }
  | { readonly state: 'unavailable'; readonly error: CoreErrorInfo };

/** 主进程发给核心进程的消息。 */
export type CoreHostInbound =
  | {
      readonly type: 'start';
      readonly nativePath: string;
      readonly userDataPath: string;
      readonly appVersion: string;
    }
  | { readonly type: 'request'; readonly id: string; readonly name: string; readonly params: unknown }
  | { readonly type: 'cancel'; readonly id: string }
  | { readonly type: 'stop' };

/** 核心进程发给主进程的消息。 */
export type CoreHostOutbound =
  | { readonly type: 'ready'; readonly coreVersion: string }
  | { readonly type: 'startFailed'; readonly error: CoreErrorInfo }
  | { readonly type: 'response'; readonly id: string; readonly ok: true; readonly result: unknown }
  | { readonly type: 'response'; readonly id: string; readonly ok: false; readonly error: CoreErrorInfo }
  | { readonly type: 'event'; readonly name: string; readonly data: unknown };

/** 带错误码的请求错误。 */
export class CoreRequestError extends Error {
  readonly code: string;

  constructor(info: CoreErrorInfo) {
    super(info.message);
    this.name = 'CoreRequestError';
    this.code = info.code;
  }
}
