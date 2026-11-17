/*
 * ------------------------------------------------------------------------
 * 名称：forward-update-status.ts
 * 说明：把升级状态的变化推送给页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：推送的内容就是 UpdateStatus（只有状态、新版本号与失败原因），页面销毁后不再发送。
 * ------------------------------------------------------------------------
 */

import type { WebContents } from 'electron';
import { SHELL_CHANNELS } from '../../../contracts/shell/shell-bridge';
import type { AppUpdater } from '../../../contracts/system/app-updater';

/**
 * 把升级状态的变化推送给页面。
 * @param updater 升级服务。
 * @param contents 页面。
 * @returns 取消转发的函数。
 */
export function forwardUpdateStatus(updater: Pick<AppUpdater, 'onStatusChange'>, contents: WebContents): () => void {
  return updater.onStatusChange((status) => {
    if (!contents.isDestroyed()) contents.send(SHELL_CHANNELS.updateStatusChanged, status);
  });
}
