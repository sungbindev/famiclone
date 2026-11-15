/*
 * ------------------------------------------------------------------------
 * 名称：open-package.ts
 * 说明：mappers.openPackage 的处理：弹出系统文件选择窗口（.emap 插件包），把用户选定的文件交给核心的 mappers.install；用户取消时返回明确的结果，不是错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：路径只来自用户在系统窗口里的选择，渲染进程不能指定；mappers.install 标 mainOnly，所以经主进程的身份发起。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { requestCoreAsMain, type CoreRequestHandling } from '../core/core-ipc';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 用户可见的文字。 */
const TEXT = {
  title: '安装 mapper 插件',
  packageFilter: 'mapper 插件包',
  allFilter: '所有文件'
} as const;

/** 插件包的扩展名。 */
const PACKAGE_EXTENSION = 'emap';

/** 安装插件处理所需的依赖。 */
export interface OpenPackageOptions {
  readonly dialog: FileDialog;
  readonly core: CoreRequestHandling;
}

/**
 * 创建 mappers.openPackage 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenPackageHandler(options: OpenPackageOptions): MainRequestHandler {
  return async (): Promise<MainResult<'mappers.openPackage'>> => {
    const path = await options.dialog.pickFile({
      title: TEXT.title,
      filters: [
        { name: TEXT.packageFilter, extensions: [PACKAGE_EXTENSION] },
        { name: TEXT.allFilter, extensions: ['*'] }
      ]
    });
    if (path === undefined) return { canceled: true };
    const result = await requestCoreAsMain(options.core, 'mappers.install', { path });
    return { canceled: false, plugin: result.plugin };
  };
}
