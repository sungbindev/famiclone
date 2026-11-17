/*
 * ------------------------------------------------------------------------
 * 名称：open-bios.ts
 * 说明：machines.openBios 的处理：弹出系统文件选择窗口（显示 .bin 与 .nes），把用户选定的文件交给核心的 machines.setBios；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：路径只来自用户在系统窗口里的选择，渲染进程不能指定；machines.setBios 标 mainOnly，所以经主进程的身份发起。
 * ------------------------------------------------------------------------
 */

import type { MainParams, MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { requestCoreAsMain, type CoreRequestHandling } from '../core/core-ipc';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 用户可见的文字。 */
const TEXT = {
  title: '添加 BIOS',
  filter: 'BIOS 文件'
} as const;

/** BIOS 文件的扩展名：纯 BIOS 文件与带 iNES 文件头的文件。 */
const BIOS_EXTENSIONS = ['bin', 'nes'];

/** 添加 BIOS 处理所需的依赖。 */
export interface OpenBiosOptions {
  readonly dialog: FileDialog;
  readonly core: CoreRequestHandling;
}

/**
 * 创建 machines.openBios 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenBiosHandler(options: OpenBiosOptions): MainRequestHandler {
  return async (params): Promise<MainResult<'machines.openBios'>> => {
    const { machine } = params as MainParams<'machines.openBios'>;
    const path = await options.dialog.pickFile({ title: TEXT.title, purpose: 'bios', filters: [{ name: TEXT.filter, extensions: BIOS_EXTENSIONS }] });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'machines.setBios', { machine, path });
    return { canceled: false, machine: result.machine };
  };
}
