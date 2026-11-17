/*
 * ------------------------------------------------------------------------
 * 名称：open-bios-directory.ts
 * 说明：machines.openBiosDirectory 的处理：请系统的文件管理器打开 BIOS 目录，打不开时返回带说明的错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：目录的位置由主进程决定，请求没有参数；实际的原因写进说明，界面用通知显示。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import type { BiosDirectory } from '../../../contracts/shell/bios-directory';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 错误码。 */
const BIOS_DIRECTORY_UNAVAILABLE = 'machines.biosDirectoryUnavailable';

/** 用户可见的文字。 */
const TEXT = {
  unavailable: (reason: string) => `无法打开 BIOS 目录：${reason}`
} as const;

/** 打开 BIOS 目录处理所需的依赖。 */
export interface OpenBiosDirectoryOptions {
  readonly biosDirectory: BiosDirectory;
}

/**
 * 创建 machines.openBiosDirectory 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenBiosDirectoryHandler(options: OpenBiosDirectoryOptions): MainRequestHandler {
  return async (): Promise<MainResult<'machines.openBiosDirectory'>> => {
    try {
      await options.biosDirectory.open();
    } catch (error) {
      throw new CoreRequestError({ code: BIOS_DIRECTORY_UNAVAILABLE, message: TEXT.unavailable(error instanceof Error ? error.message : String(error)) });
    }
    return {};
  };
}
