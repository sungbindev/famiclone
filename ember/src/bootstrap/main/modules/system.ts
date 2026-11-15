/*
 * ------------------------------------------------------------------------
 * 名称：system.ts
 * 说明：系统模块在主进程中的登记项：Schema，以及主进程自己处理的 system.openLogDirectory（用系统的文件管理器打开日志目录）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { SYSTEM_MAIN_REQUESTS, SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import { createOpenLogDirectoryHandler } from '../../../entry/ipc/system/open-log-directory';
import type { MainModule } from '../main-module';

/** 系统模块。 */
export const SYSTEM_MAIN_MODULE: MainModule = {
  schema: SYSTEM_SCHEMA,
  createMainHandlers: (context) => ({
    [SYSTEM_MAIN_REQUESTS.openLogDirectory]: createOpenLogDirectoryHandler({ logDirectory: context.logDirectory })
  })
};
