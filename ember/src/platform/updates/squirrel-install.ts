/*
 * ------------------------------------------------------------------------
 * 名称：squirrel-install.ts
 * 说明：判断应用是不是用 Squirrel.Windows 安装的：安装目录下有 Update.exe 与根目录的启动器，当前程序在 app-<版本> 目录里；是则返回启动器的路径，否则返回 undefined。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：只有这种安装方式的应用才能用 Squirrel 升级；开发版、免安装的打包目录与其他系统都不是。启动器是安装根目录里与可执行文件同名的小程序，它总是启动最新的 app-<版本> 目录，重启升级时用它。
 *       文件是否存在由调用方传入，便于测试；路径规则按 Windows 处理。
 * ------------------------------------------------------------------------
 */

import { win32 } from 'node:path';

/** Squirrel 的更新程序文件名。 */
const UPDATE_EXE = 'Update.exe';
/** 当前版本目录的名字格式。 */
const VERSION_DIRECTORY = /^app-/i;

/** 判断所需的环境信息。 */
export interface SquirrelInstallProbe {
  /** 操作系统（process.platform）。 */
  readonly platform: string;
  /** 是否是打包后的应用。 */
  readonly isPackaged: boolean;
  /** 当前可执行文件的路径（process.execPath）。 */
  readonly execPath: string;
  /** 路径是否存在。 */
  readonly exists: (path: string) => boolean;
}

/**
 * 找到 Squirrel 安装的启动器。
 * @param probe 环境信息。
 * @returns 启动器的路径；不是 Squirrel 安装的应用时为 undefined。
 */
export function findSquirrelLauncher(probe: SquirrelInstallProbe): string | undefined {
  if (probe.platform !== 'win32' || !probe.isPackaged) return undefined;
  const versionDirectory = win32.dirname(probe.execPath);
  if (!VERSION_DIRECTORY.test(win32.basename(versionDirectory))) return undefined;
  const root = win32.dirname(versionDirectory);
  const launcher = win32.join(root, win32.basename(probe.execPath));
  return probe.exists(win32.join(root, UPDATE_EXE)) && probe.exists(launcher) ? launcher : undefined;
}
