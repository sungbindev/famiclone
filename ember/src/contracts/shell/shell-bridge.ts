/*
 * ------------------------------------------------------------------------
 * 名称：shell-bridge.ts
 * 说明：外壳与主进程之间的约定：IPC 通道名、核心请求的返回格式，以及预加载脚本暴露给界面的 window.ember 的类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：界面只经 window.ember 与主进程通信；核心请求按 Schema 生成的 CoreApi 暴露为具名方法，没有通用的调用入口；
 *       主进程自己处理的请求（如打开卡带的文件选择）按生成的 MainApi 暴露，画面与输入的端口经 streams 取得。
 * ------------------------------------------------------------------------
 */

import type { CoreErrorInfo, CoreStatus } from '../core-gateway/core-messages';
import type { CoreApi, MainApi } from '../core-gateway/generated/core-api';
import type { StreamPortsBridge } from '../stream-ports/stream-ports';

/** IPC 通道名。 */
export const SHELL_CHANNELS = {
  coreRequest: 'core:request',
  coreEvent: 'core:event',
  coreStatus: 'core:status',
  mainRequest: 'main:request'
} as const;

/** 核心请求的 IPC 返回：成功时为结果，失败时为错误码与说明。 */
export type CoreRequestReply = { readonly ok: true; readonly result: unknown } | { readonly ok: false; readonly error: CoreErrorInfo };

/** 预加载脚本暴露给界面的接口（window.ember）。 */
export interface EmberBridge {
  /** 核心请求，按模块分组的具名方法；失败时以 CoreErrorInfo（{code, message}）拒绝。 */
  readonly core: CoreApi;
  /** 主进程自己处理的请求，按模块分组的具名方法（如 cartridges.open 弹出文件选择窗口）；失败时同样以 CoreErrorInfo 拒绝。 */
  readonly main: MainApi;
  /** 画面与输入的消息端口。 */
  readonly streams: StreamPortsBridge;
  /** 订阅核心事件；返回取消订阅的函数。 */
  onCoreEvent(listener: (name: string, data: unknown) => void): () => void;
  /** 核心的运行状态。 */
  getCoreStatus(): Promise<CoreStatus>;
}
