/*
 * ------------------------------------------------------------------------
 * 名称：pick-directory.ts
 * 说明：preferences.pickDirectory 的处理：弹出系统的选择文件夹窗口，返回用户选定的文件夹路径；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：路径只来自用户在系统窗口里的选择；返回给页面后由页面经 preferences.set 保存，本处理不访问核心。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 用户可见的文字。 */
const TEXT = {
  title: '选择文件夹'
} as const;

/** 选择文件夹处理所需的依赖。 */
export interface PickDirectoryOptions {
  readonly dialog: FileDialog;
}

/**
 * 创建 preferences.pickDirectory 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createPickDirectoryHandler(options: PickDirectoryOptions): MainRequestHandler {
  return async (): Promise<MainResult<'preferences.pickDirectory'>> => {
    const path = await options.dialog.pickDirectory({ title: TEXT.title, purpose: 'folder' });
    return path === undefined ? { canceled: true } : { canceled: false, path };
  };
}
