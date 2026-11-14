/*
 * ------------------------------------------------------------------------
 * 名称：preferences.ts
 * 说明：首选项模块在主进程中的登记项：Schema，以及主进程自己处理的 preferences.pickDirectory（弹出选择文件夹窗口）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { PREFERENCES_MAIN_REQUESTS, PREFERENCES_SCHEMA } from '../../../contracts/preferences/generated/preferences';
import { createPickDirectoryHandler } from '../../../entry/ipc/preferences/pick-directory';
import type { MainModule } from '../main-module';

/** 首选项模块。 */
export const PREFERENCES_MAIN_MODULE: MainModule = {
  schema: PREFERENCES_SCHEMA,
  createMainHandlers: (context) => ({
    [PREFERENCES_MAIN_REQUESTS.pickDirectory]: createPickDirectoryHandler({ dialog: context.fileDialog })
  })
};
