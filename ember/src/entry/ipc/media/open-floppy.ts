/*
 * ------------------------------------------------------------------------
 * 名称：open-floppy.ts
 * 说明：media.openFloppy 的处理：弹出系统文件选择窗口（软盘镜像），把用户选定的文件交给核心的 media.floppyInsert；给出 droppedId 时不弹窗口，用拖入窗口的那个镜像文件；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：路径只来自用户在系统窗口里的选择或拖入窗口的文件（编号由主进程登记，路径取自预加载脚本），渲染进程不能指定；media.floppyInsert 标 mainOnly，所以经主进程的身份发起。
 * ------------------------------------------------------------------------
 */

import type { MainParams, MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { DroppedFilesService } from '../../../contracts/shell/dropped-files';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { requestCoreAsMain, type CoreRequestHandling } from '../core/core-ipc';
import { resolveDroppedPath } from '../core/dropped-path';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 用户可见的文字。 */
const TEXT = {
  title: '载入软盘镜像',
  imageFilter: '软盘镜像',
  allFilter: '所有文件',
  droppedNotFound: '拖入的文件已失效或不是软盘镜像，请重新拖入'
} as const;

/** 软盘镜像常见的扩展名。 */
const FLOPPY_EXTENSIONS = ['img', 'ima', 'vfd', 'dsk', 'flp'];

/** 找不到拖入的文件时的错误码。 */
const DROPPED_NOT_FOUND = 'media.droppedNotFound';

/** 载入软盘镜像处理所需的依赖。 */
export interface OpenFloppyOptions {
  readonly dialog: FileDialog;
  readonly dropped: DroppedFilesService;
  readonly core: CoreRequestHandling;
}

/**
 * 创建 media.openFloppy 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenFloppyHandler(options: OpenFloppyOptions): MainRequestHandler {
  return async (params): Promise<MainResult<'media.openFloppy'>> => {
    const { drive, droppedId } = params as MainParams<'media.openFloppy'>;
    const path =
      droppedId === undefined
        ? await options.dialog.pickFile({
            title: TEXT.title,
            filters: [
              { name: TEXT.imageFilter, extensions: FLOPPY_EXTENSIONS },
              { name: TEXT.allFilter, extensions: ['*'] }
            ]
          })
        : resolveDroppedPath(options.dropped, droppedId, { extensions: FLOPPY_EXTENSIONS, notFound: { code: DROPPED_NOT_FOUND, message: TEXT.droppedNotFound } });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'media.floppyInsert', { drive, path });
    return { canceled: false, status: result.status };
  };
}
