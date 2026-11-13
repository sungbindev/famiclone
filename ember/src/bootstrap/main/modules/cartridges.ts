/*
 * ------------------------------------------------------------------------
 * 名称：cartridges.ts
 * 说明：卡带模块在主进程中的登记项：Schema，以及主进程自己处理的 cartridges.open（弹出文件选择窗口后加载卡带）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { CARTRIDGES_MAIN_REQUESTS, CARTRIDGES_SCHEMA } from '../../../contracts/cartridges/generated/cartridges';
import { createOpenCartridgeHandler } from '../../../entry/ipc/cartridges/open-cartridge';
import type { MainModule } from '../main-module';

/** 卡带模块。 */
export const CARTRIDGES_MAIN_MODULE: MainModule = {
  schema: CARTRIDGES_SCHEMA,
  createMainHandlers: (context) => ({
    [CARTRIDGES_MAIN_REQUESTS.open]: createOpenCartridgeHandler({ dialog: context.fileDialog, core: context.core })
  })
};
