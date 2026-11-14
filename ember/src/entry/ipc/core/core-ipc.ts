/*
 * ------------------------------------------------------------------------
 * 名称：core-ipc.ts
 * 说明：核心请求的 IPC 处理：检查发送方，只接受已登记模块的请求名，按 Schema 校验参数后交给核心网关；把核心事件转发给界面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：所有模块共用这一个处理函数，新增模块只在主进程的模块清单中登记；返回值带错误码，因为 IPC 抛出的异常会丢失错误码。
 * ------------------------------------------------------------------------
 */

import type { IpcMain, WebContents } from 'electron';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CORE_ERROR_CODES, CoreRequestError, type CoreErrorInfo } from '../../../contracts/core-gateway/core-messages';
import type { CoreParams, CoreRequestName, CoreResult } from '../../../contracts/core-gateway/generated/core-api';
import { SHELL_CHANNELS, type CoreRequestReply } from '../../../contracts/shell/shell-bridge';
import type { ModuleSchema } from '../../../shared/schema/schema-types';
import { validateSchema } from '../../../shared/schema/validate-schema';
import type { SenderCheck } from '../sender-check';

/** 用户可见的说明。 */
const TEXT = {
  untrusted: '不接受来自应用以外页面的请求',
  unknownRequest: '没有这个请求',
  invalidParams: '请求参数不正确',
  internal: '请求失败，详情已写入日志'
} as const;

/** 核心请求 IPC 的依赖。 */
export interface CoreIpcOptions {
  readonly ipc: IpcMain;
  readonly gateway: CoreGateway;
  /** 已登记的模块的 Schema；不在其中的请求直接拒绝。 */
  readonly modules: readonly ModuleSchema[];
  /** 全部模块的 Schema，用于跨模块的类型引用。 */
  readonly schemas: readonly ModuleSchema[];
  readonly isTrustedSender: SenderCheck;
}

/** 处理一次请求所需的依赖；主进程自己发起时 allowMainOnly 为 true。 */
export type CoreRequestHandling = Omit<CoreIpcOptions, 'ipc' | 'isTrustedSender'> & { readonly allowMainOnly?: boolean };

/** 失败的返回。 */
function failure(error: CoreErrorInfo): CoreRequestReply {
  return { ok: false, error };
}

/**
 * 处理一次核心请求；导出供测试使用。
 * @returns 成功时为结果，失败时为错误码与说明。
 */
export async function handleCoreRequest(options: CoreRequestHandling, name: unknown, params: unknown): Promise<CoreRequestReply> {
  if (typeof name !== 'string') return failure({ code: CORE_ERROR_CODES.unknownRequest, message: TEXT.unknownRequest });
  const dot = name.indexOf('.');
  const module = options.modules.find((schema) => schema.module === name.slice(0, dot));
  const request = dot > 0 && module !== undefined ? module.requests[name.slice(dot + 1)] : undefined;
  if (module === undefined || request === undefined || (request.mainOnly === true && options.allowMainOnly !== true) || request.debugOnly === true) {
    return failure({ code: CORE_ERROR_CODES.unknownRequest, message: `${TEXT.unknownRequest}：${name}` });
  }

  const validation = validateSchema(request.params, params, module, options.schemas);
  if (!validation.valid) return failure({ code: CORE_ERROR_CODES.invalidParams, message: `${TEXT.invalidParams}：${validation.errors.join('；')}` });

  try {
    const result = await options.gateway.request(name as CoreRequestName, params as CoreParams<CoreRequestName>);
    return { ok: true, result };
  } catch (error) {
    if (error instanceof CoreRequestError) return failure({ code: error.code, message: error.message });
    return failure({ code: CORE_ERROR_CODES.internal, message: TEXT.internal });
  }
}

/**
 * 以主进程的身份发起核心请求（可以发起只能由主进程发起的请求），同样按 Schema 校验参数。
 * @throws CoreRequestError 请求失败时，带错误码与说明。
 */
export async function requestCoreAsMain<N extends CoreRequestName>(options: CoreRequestHandling, name: N, params: CoreParams<N>): Promise<CoreResult<N>> {
  const reply = await handleCoreRequest({ ...options, allowMainOnly: true }, name, params);
  if (!reply.ok) throw new CoreRequestError(reply.error);
  return reply.result as CoreResult<N>;
}

/** 登记核心请求的 IPC 处理。 */
export function registerCoreIpc(options: CoreIpcOptions): void {
  options.ipc.handle(SHELL_CHANNELS.coreRequest, (event, name: unknown, params: unknown) => {
    if (!options.isTrustedSender(event)) return failure({ code: CORE_ERROR_CODES.unknownRequest, message: TEXT.untrusted });
    return handleCoreRequest(options, name, params);
  });
  options.ipc.handle(SHELL_CHANNELS.coreStatus, (event) => (options.isTrustedSender(event) ? options.gateway.status() : undefined));
}

/**
 * 把核心事件与核心运行状态的变化转发给窗口。
 * @returns 停止转发的函数。
 */
export function forwardCoreToWindow(gateway: CoreGateway, contents: WebContents): () => void {
  const stopEvents = gateway.onEvent((name, data) => {
    if (!contents.isDestroyed()) contents.send(SHELL_CHANNELS.coreEvent, name, data);
  });
  const stopStatus = gateway.onStatusChange((status) => {
    if (!contents.isDestroyed()) contents.send(SHELL_CHANNELS.coreStatusChanged, status);
  });
  return () => {
    stopEvents();
    stopStatus();
  };
}
