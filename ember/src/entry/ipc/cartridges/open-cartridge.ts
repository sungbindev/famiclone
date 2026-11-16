/*
 * ------------------------------------------------------------------------
 * 名称：open-cartridge.ts
 * 说明：cartridges.open 的处理：弹出系统文件选择窗口（只显示 .nes），把用户选定的路径交给核心的 cartridges.load 并记入最近打开；给出 droppedId 时不弹窗口，用拖入窗口的那个 .nes 文件；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只来自用户在系统窗口里的选择或拖入窗口的文件（编号由主进程登记，路径取自预加载脚本），渲染进程不能指定；cartridges.load 标 mainOnly，所以经主进程的身份发起。
 * ------------------------------------------------------------------------
 */

import type { MainParams, MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { DroppedFilesService } from '../../../contracts/shell/dropped-files';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { resolveDroppedPath } from '../core/dropped-path';
import type { MainRequestHandler } from '../core/main-request-ipc';
import { loadAndRemember, type LoadCartridgeOptions } from './load-cartridge';

/** 用户可见的文字。 */
const TEXT = {
  title: '打开卡带',
  filter: 'NES 卡带',
  droppedNotFound: '拖入的文件已失效或不是 NES 卡带，请重新拖入'
} as const;

/** 卡带文件的扩展名。 */
const CARTRIDGE_EXTENSION = 'nes';

/** 找不到拖入的文件时的错误码。 */
const DROPPED_NOT_FOUND = 'cartridges.droppedNotFound';

/** 打开卡带处理所需的依赖。 */
export interface OpenCartridgeOptions extends LoadCartridgeOptions {
  readonly dialog: FileDialog;
  readonly dropped: DroppedFilesService;
}

/**
 * 创建 cartridges.open 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenCartridgeHandler(options: OpenCartridgeOptions): MainRequestHandler {
  return async (params): Promise<MainResult<'cartridges.open'>> => {
    const { droppedId } = params as MainParams<'cartridges.open'>;
    const path =
      droppedId === undefined
        ? await options.dialog.pickFile({ title: TEXT.title, purpose: 'cartridge', filters: [{ name: TEXT.filter, extensions: [CARTRIDGE_EXTENSION] }] })
        : resolveDroppedPath(options.dropped, droppedId, { extensions: [CARTRIDGE_EXTENSION], notFound: { code: DROPPED_NOT_FOUND, message: TEXT.droppedNotFound } });
    if (path === undefined) return { canceled: true };
    const loaded = await loadAndRemember(options, path);
    return { canceled: false, loaded };
  };
}
