/*
 * ------------------------------------------------------------------------
 * 名称：system.ts
 * 说明：系统模块在主进程中的登记项：Schema，以及主进程自己处理的 system.openLogDirectory（用系统的文件管理器打开日志目录）、system.openHelp（在应用内打开帮助文档窗口）与升级的三个请求 system.updateStatus、system.checkUpdate、system.installUpdate。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * ------------------------------------------------------------------------
 */

import { SYSTEM_MAIN_REQUESTS, SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import { createOpenHelpHandler } from '../../../entry/ipc/system/open-help';
import { createOpenLogDirectoryHandler } from '../../../entry/ipc/system/open-log-directory';
import { createCheckUpdateHandler, createInstallUpdateHandler, createUpdateStatusHandler } from '../../../entry/ipc/system/update-requests';
import type { MainModule } from '../main-module';

/** 系统模块。 */
export const SYSTEM_MAIN_MODULE: MainModule = {
  schema: SYSTEM_SCHEMA,
  createMainHandlers: (context) => ({
    [SYSTEM_MAIN_REQUESTS.openLogDirectory]: createOpenLogDirectoryHandler({ logDirectory: context.logDirectory }),
    [SYSTEM_MAIN_REQUESTS.openHelp]: createOpenHelpHandler({ helpWindow: context.helpWindow }),
    [SYSTEM_MAIN_REQUESTS.updateStatus]: createUpdateStatusHandler({ updater: context.updater }),
    [SYSTEM_MAIN_REQUESTS.checkUpdate]: createCheckUpdateHandler({ updater: context.updater }),
    [SYSTEM_MAIN_REQUESTS.installUpdate]: createInstallUpdateHandler({ updater: context.updater })
  })
};
