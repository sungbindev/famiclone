/*
 * ------------------------------------------------------------------------
 * 名称：open-tape.ts
 * 说明：media.openTape 与 media.openTapeRecording 的处理：弹出系统的文件窗口（选声音文件，或选保存录音的 WAV 文件），把用户选定的路径交给核心的 media.tapeLoad 与 media.tapeRecord；media.openTape 给出 droppedId 时不弹窗口，用拖入窗口的那个声音文件；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：路径只来自用户在系统窗口里的选择或拖入窗口的文件（编号由主进程登记，路径取自预加载脚本），渲染进程不能指定；media.tapeLoad 与 media.tapeRecord 标 mainOnly，所以经主进程的身份发起。
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
  loadTitle: '载入磁带',
  soundFilter: '声音文件',
  recordTitle: '保存录音',
  wavFilter: 'WAV 声音文件',
  allFilter: '所有文件',
  droppedNotFound: '拖入的文件已失效或不是声音文件，请重新拖入'
} as const;

/** 磁带可以用的声音文件扩展名（ffmpeg 能解码的常见格式）。 */
const SOUND_EXTENSIONS = ['m4a', 'mp3', 'wav', 'aac', 'flac', 'ogg', 'opus', 'mp4', 'mka'];

/** 找不到拖入的文件时的错误码。 */
const DROPPED_NOT_FOUND = 'media.droppedNotFound';

/** 载入磁带处理所需的依赖。 */
export interface OpenTapeOptions {
  readonly dialog: FileDialog;
  readonly dropped: DroppedFilesService;
  readonly core: CoreRequestHandling;
}

/** 创建 media.openTape 的处理函数。 */
export function createOpenTapeHandler(options: OpenTapeOptions): MainRequestHandler {
  return async (params): Promise<MainResult<'media.openTape'>> => {
    const { droppedId } = params as MainParams<'media.openTape'>;
    const path =
      droppedId === undefined
        ? await options.dialog.pickFile({
            title: TEXT.loadTitle,
            filters: [
              { name: TEXT.soundFilter, extensions: SOUND_EXTENSIONS },
              { name: TEXT.allFilter, extensions: ['*'] }
            ]
          })
        : resolveDroppedPath(options.dropped, droppedId, { extensions: SOUND_EXTENSIONS, notFound: { code: DROPPED_NOT_FOUND, message: TEXT.droppedNotFound } });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'media.tapeLoad', { path });
    return { canceled: false, status: result.status };
  };
}

/** 创建 media.openTapeRecording 的处理函数。 */
export function createOpenTapeRecordingHandler(options: Omit<OpenTapeOptions, 'dropped'>): MainRequestHandler {
  return async (): Promise<MainResult<'media.openTapeRecording'>> => {
    const path = await options.dialog.pickSaveFile({ title: TEXT.recordTitle, filters: [{ name: TEXT.wavFilter, extensions: ['wav'] }] });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'media.tapeRecord', { path });
    return { canceled: false, status: result.status };
  };
}
