/*
 * ------------------------------------------------------------------------
 * 名称：open-cartridge.ts
 * 说明：cartridges.open 的处理：弹出系统文件选择窗口（只显示 .nes），把用户选定的路径交给核心的 cartridges.load 并记入最近打开；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只来自用户在系统窗口里的选择，渲染进程不能指定；cartridges.load 标 mainOnly，所以经主进程的身份发起。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import type { MainRequestHandler } from '../core/main-request-ipc';
import { loadAndRemember, type LoadCartridgeOptions } from './load-cartridge';

/** 用户可见的文字。 */
const TEXT = {
  title: '打开卡带',
  filter: 'NES 卡带'
} as const;

/** 卡带文件的扩展名。 */
const CARTRIDGE_EXTENSION = 'nes';

/** 打开卡带处理所需的依赖。 */
export interface OpenCartridgeOptions extends LoadCartridgeOptions {
  readonly dialog: FileDialog;
}

/**
 * 创建 cartridges.open 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenCartridgeHandler(options: OpenCartridgeOptions): MainRequestHandler {
  return async (): Promise<MainResult<'cartridges.open'>> => {
    const path = await options.dialog.pickFile({ title: TEXT.title, filters: [{ name: TEXT.filter, extensions: [CARTRIDGE_EXTENSION] }] });
    if (path === undefined) return { canceled: true };
    const loaded = await loadAndRemember(options, path);
    return { canceled: false, loaded };
  };
}
