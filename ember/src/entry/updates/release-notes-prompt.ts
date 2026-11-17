/*
 * ------------------------------------------------------------------------
 * 名称：release-notes-prompt.ts
 * 说明：升级后第一次启动新版本时，自动打开帮助文档里的“更新记录”页；每个版本只提示一次。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：只有支持升级的运行方式（Squirrel 安装版）才提示，开发版与免安装目录不提示；
 *       Squirrel 安装后第一次启动（firstRun）是全新安装，只记下当前版本、不提示；没有记录又不是全新安装，说明是从不会记录的旧版本升上来的，要提示；
 *       先记下版本再打开页面，页面打不开也不会每次启动都重试。
 * ------------------------------------------------------------------------
 */

import type { HelpWindow } from '../../contracts/shell/help-window';
import type { ShellStateService } from '../../contracts/shell/shell-state-service';
import type { AppUpdater } from '../../contracts/system/app-updater';

/** 提示更新记录所需的依赖。 */
export interface ReleaseNotesPromptOptions {
  readonly updater: Pick<AppUpdater, 'status'>;
  /** 当前运行的应用版本。 */
  readonly currentVersion: string;
  /** 是否是 Squirrel 安装后的第一次启动。 */
  readonly firstRun: boolean;
  readonly state: Pick<ShellStateService, 'get' | 'updateReleaseNotesVersion'>;
  readonly helpWindow: Pick<HelpWindow, 'open'>;
  /** 打开页面失败时调用，不影响运行。 */
  readonly onError?: (error: unknown) => void;
}

/**
 * 需要时打开更新记录页，并记下已经提示过的版本。
 * @param options 依赖。
 */
export function showReleaseNotesOnce(options: ReleaseNotesPromptOptions): void {
  if (options.updater.status().state === 'unsupported') return;
  if (options.state.get().releaseNotesVersion === options.currentVersion) return;
  options.state.updateReleaseNotesVersion(options.currentVersion);
  if (options.firstRun) return;
  options.helpWindow.open('changelog').catch((error: unknown) => options.onError?.(error));
}
