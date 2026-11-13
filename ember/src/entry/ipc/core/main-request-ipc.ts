/*
 * ------------------------------------------------------------------------
 * 名称：main-request-ipc.ts
 * 说明：主进程自己处理的请求（mainRequests）的 IPC 入口：检查发送方，只接受已登记模块、已登记处理函数的请求名，按 Schema 校验参数后交给处理函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：这类请求不进入核心（如弹出系统文件选择窗口再请核心加载）；所有模块共用这一个入口，处理函数由各模块在组装根登记；
 *       返回值带错误码，因为 IPC 抛出的异常会丢失错误码。
 * ------------------------------------------------------------------------
 */

import type { IpcMain } from 'electron';
import { CORE_ERROR_CODES, CoreRequestError, type CoreErrorInfo } from '../../../contracts/core-gateway/core-messages';
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

/** 主进程请求的处理函数：参数已按 Schema 校验；失败时抛出 CoreRequestError。 */
export type MainRequestHandler = (params: unknown) => Promise<unknown>;

/** 主进程请求处理所需的依赖。 */
export interface MainRequestHandling {
  /** 已登记的处理函数，键为请求名“模块.操作”。 */
  readonly handlers: Readonly<Record<string, MainRequestHandler>>;
  /** 已登记的模块的 Schema；不在其中的请求直接拒绝。 */
  readonly modules: readonly ModuleSchema[];
  /** 全部模块的 Schema，用于跨模块的类型引用。 */
  readonly schemas: readonly ModuleSchema[];
}

/** 主进程请求 IPC 的依赖。 */
export interface MainRequestIpcOptions extends MainRequestHandling {
  readonly ipc: IpcMain;
  readonly isTrustedSender: SenderCheck;
  /** 处理函数抛出非 CoreRequestError 的异常时调用，用于写日志。 */
  readonly onError?: (error: unknown, name: string) => void;
}

/** 失败的返回。 */
function failure(error: CoreErrorInfo): CoreRequestReply {
  return { ok: false, error };
}

/**
 * 处理一次主进程请求；导出供测试使用。
 * @returns 成功时为结果，失败时为错误码与说明。
 */
export async function handleMainRequest(options: MainRequestHandling & Pick<MainRequestIpcOptions, 'onError'>, name: unknown, params: unknown): Promise<CoreRequestReply> {
  if (typeof name !== 'string') return failure({ code: CORE_ERROR_CODES.unknownRequest, message: TEXT.unknownRequest });
  const dot = name.indexOf('.');
  const module = dot > 0 ? options.modules.find((schema) => schema.module === name.slice(0, dot)) : undefined;
  const request = module?.mainRequests?.[name.slice(dot + 1)];
  const handler = Object.hasOwn(options.handlers, name) ? options.handlers[name] : undefined;
  if (module === undefined || request === undefined || handler === undefined) return failure({ code: CORE_ERROR_CODES.unknownRequest, message: `${TEXT.unknownRequest}：${name}` });

  const validation = validateSchema(request.params, params, module, options.schemas);
  if (!validation.valid) return failure({ code: CORE_ERROR_CODES.invalidParams, message: `${TEXT.invalidParams}：${validation.errors.join('；')}` });

  try {
    return { ok: true, result: await handler(params) };
  } catch (error) {
    if (error instanceof CoreRequestError) return failure({ code: error.code, message: error.message });
    options.onError?.(error, name);
    return failure({ code: CORE_ERROR_CODES.internal, message: TEXT.internal });
  }
}

/** 登记主进程请求的 IPC 处理。 */
export function registerMainRequestIpc(options: MainRequestIpcOptions): void {
  options.ipc.handle(SHELL_CHANNELS.mainRequest, (event, name: unknown, params: unknown) => {
    if (!options.isTrustedSender(event)) return failure({ code: CORE_ERROR_CODES.unknownRequest, message: TEXT.untrusted });
    return handleMainRequest(options, name, params);
  });
}
