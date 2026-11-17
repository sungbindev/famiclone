/*
 * ------------------------------------------------------------------------
 * 名称：machines.ts
 * 说明：机型模块在主进程中的登记项：Schema，以及主进程自己处理的 machines.openBios（弹出文件选择窗口后添加 BIOS）与 machines.openBiosDirectory（用文件管理器打开 BIOS 目录）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { MACHINES_MAIN_REQUESTS, MACHINES_SCHEMA } from '../../../contracts/machines/generated/machines';
import { createOpenBiosHandler } from '../../../entry/ipc/machines/open-bios';
import { createOpenBiosDirectoryHandler } from '../../../entry/ipc/machines/open-bios-directory';
import type { MainModule } from '../main-module';

/** 机型模块。 */
export const MACHINES_MAIN_MODULE: MainModule = {
  schema: MACHINES_SCHEMA,
  createMainHandlers: (context) => ({
    [MACHINES_MAIN_REQUESTS.openBios]: createOpenBiosHandler({ dialog: context.fileDialog, core: context.core }),
    [MACHINES_MAIN_REQUESTS.openBiosDirectory]: createOpenBiosDirectoryHandler({ biosDirectory: context.biosDirectory })
  })
};
