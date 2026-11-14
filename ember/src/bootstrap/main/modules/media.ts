/*
 * ------------------------------------------------------------------------
 * 名称：media.ts
 * 说明：介质模块在主进程中的登记项：Schema，以及主进程自己处理的 media.openFloppy（弹出文件选择窗口后载入软盘镜像）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { MEDIA_MAIN_REQUESTS, MEDIA_SCHEMA } from '../../../contracts/media/generated/media';
import { createOpenFloppyHandler } from '../../../entry/ipc/media/open-floppy';
import type { MainModule } from '../main-module';

/** 介质模块。 */
export const MEDIA_MAIN_MODULE: MainModule = {
  schema: MEDIA_SCHEMA,
  createMainHandlers: (context) => ({
    [MEDIA_MAIN_REQUESTS.openFloppy]: createOpenFloppyHandler({ dialog: context.fileDialog, core: context.core })
  })
};
