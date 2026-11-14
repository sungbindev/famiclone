/*
 * ------------------------------------------------------------------------
 * 名称：cartridges.ts
 * 说明：卡带模块在主进程中的登记项：Schema，以及主进程自己处理的 cartridges.open（弹出文件选择窗口后加载卡带）、cartridges.recent（最近打开的清单）与 cartridges.openRecent。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { CARTRIDGES_MAIN_REQUESTS, CARTRIDGES_SCHEMA } from '../../../contracts/cartridges/generated/cartridges';
import { createOpenCartridgeHandler } from '../../../entry/ipc/cartridges/open-cartridge';
import { createOpenRecentCartridgeHandler, createRecentCartridgesHandler } from '../../../entry/ipc/cartridges/recent-cartridges';
import type { MainModule } from '../main-module';

/** 卡带模块。 */
export const CARTRIDGES_MAIN_MODULE: MainModule = {
  schema: CARTRIDGES_SCHEMA,
  createMainHandlers: (context) => {
    const loading = { core: context.core, recent: context.recentCartridges };
    return {
      [CARTRIDGES_MAIN_REQUESTS.open]: createOpenCartridgeHandler({ dialog: context.fileDialog, ...loading }),
      [CARTRIDGES_MAIN_REQUESTS.recent]: createRecentCartridgesHandler(loading),
      [CARTRIDGES_MAIN_REQUESTS.openRecent]: createOpenRecentCartridgeHandler(loading)
    };
  }
};
