/*
 * ------------------------------------------------------------------------
 * 名称：open-log-directory.ts
 * 说明：system.openLogDirectory 的处理：请系统的文件管理器打开日志目录，打不开时返回带说明的错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：目录的位置由主进程决定，请求没有参数；实际的原因写进说明，界面用通知显示。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import type { LogDirectory } from '../../../contracts/shell/log-directory';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 错误码。 */
const LOG_DIRECTORY_UNAVAILABLE = 'system.logDirectoryUnavailable';

/** 用户可见的文字。 */
const TEXT = {
  unavailable: (reason: string) => `无法打开日志目录：${reason}`
} as const;

/** 打开日志目录处理所需的依赖。 */
export interface OpenLogDirectoryOptions {
  readonly logDirectory: LogDirectory;
}

/**
 * 创建 system.openLogDirectory 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenLogDirectoryHandler(options: OpenLogDirectoryOptions): MainRequestHandler {
  return async (): Promise<MainResult<'system.openLogDirectory'>> => {
    try {
      await options.logDirectory.open();
    } catch (error) {
      throw new CoreRequestError({ code: LOG_DIRECTORY_UNAVAILABLE, message: TEXT.unavailable(error instanceof Error ? error.message : String(error)) });
    }
    return {};
  };
}
