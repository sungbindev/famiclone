/*
 * ------------------------------------------------------------------------
 * 名称：log-directory.ts
 * 说明：日志目录的实现：目录不存在时先创建，再用传入的函数（系统的文件管理器）打开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：Electron 的 shell.openPath 打不开时返回错误说明而不是抛出异常，这里统一抛出；打开函数由组装根传入，所以本文件不引用 electron。
 * ------------------------------------------------------------------------
 */

import { mkdir } from 'node:fs/promises';

/** 可以用系统的文件管理器打开的目录。 */
export interface OpenableDirectory {
  open(): Promise<void>;
}

/**
 * 创建日志目录。
 * @param path 日志目录的路径。
 * @param openPath 打开路径的函数，返回空字符串表示成功，否则是错误说明（同 Electron 的 shell.openPath）。
 */
export function createLogDirectory(path: string, openPath: (target: string) => Promise<string>): OpenableDirectory {
  return {
    open: async () => {
      await mkdir(path, { recursive: true });
      const failure = await openPath(path);
      if (failure !== '') throw new Error(failure);
    }
  };
}
