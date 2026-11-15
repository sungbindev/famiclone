/*
 * ------------------------------------------------------------------------
 * 名称：mappers.ts
 * 说明：mapper 插件模块在主进程中的登记项：Schema，以及主进程自己处理的 mappers.openPackage（弹出文件选择窗口后安装插件包）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { MAPPERS_MAIN_REQUESTS, MAPPERS_SCHEMA } from '../../../contracts/mappers/generated/mappers';
import { createOpenPackageHandler } from '../../../entry/ipc/mappers/open-package';
import type { MainModule } from '../main-module';

/** mapper 插件模块。 */
export const MAPPERS_MAIN_MODULE: MainModule = {
  schema: MAPPERS_SCHEMA,
  createMainHandlers: (context) => ({
    [MAPPERS_MAIN_REQUESTS.openPackage]: createOpenPackageHandler({ dialog: context.fileDialog, core: context.core })
  })
};
