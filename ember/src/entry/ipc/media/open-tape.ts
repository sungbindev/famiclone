/*
 * ------------------------------------------------------------------------
 * 名称：open-tape.ts
 * 说明：media.openTape 与 media.openTapeRecording 的处理：弹出系统的文件窗口（选声音文件，或选保存录音的 WAV 文件），把用户选定的路径交给核心的 media.tapeLoad 与 media.tapeRecord；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：路径只来自用户在系统窗口里的选择，渲染进程不能指定；media.tapeLoad 与 media.tapeRecord 标 mainOnly，所以经主进程的身份发起。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { requestCoreAsMain, type CoreRequestHandling } from '../core/core-ipc';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 用户可见的文字。 */
const TEXT = {
  loadTitle: '载入磁带',
  soundFilter: '声音文件',
  recordTitle: '保存录音',
  wavFilter: 'WAV 声音文件',
  allFilter: '所有文件'
} as const;

/** 磁带可以用的声音文件扩展名（ffmpeg 能解码的常见格式）。 */
const SOUND_EXTENSIONS = ['m4a', 'mp3', 'wav', 'aac', 'flac', 'ogg', 'opus', 'mp4', 'mka'];

/** 载入磁带处理所需的依赖。 */
export interface OpenTapeOptions {
  readonly dialog: FileDialog;
  readonly core: CoreRequestHandling;
}

/** 创建 media.openTape 的处理函数。 */
export function createOpenTapeHandler(options: OpenTapeOptions): MainRequestHandler {
  return async (): Promise<MainResult<'media.openTape'>> => {
    const path = await options.dialog.pickFile({
      title: TEXT.loadTitle,
      filters: [
        { name: TEXT.soundFilter, extensions: SOUND_EXTENSIONS },
        { name: TEXT.allFilter, extensions: ['*'] }
      ]
    });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'media.tapeLoad', { path });
    return { canceled: false, status: result.status };
  };
}

/** 创建 media.openTapeRecording 的处理函数。 */
export function createOpenTapeRecordingHandler(options: OpenTapeOptions): MainRequestHandler {
  return async (): Promise<MainResult<'media.openTapeRecording'>> => {
    const path = await options.dialog.pickSaveFile({ title: TEXT.recordTitle, filters: [{ name: TEXT.wavFilter, extensions: ['wav'] }] });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'media.tapeRecord', { path });
    return { canceled: false, status: result.status };
  };
}
