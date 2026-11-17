/*
 * ------------------------------------------------------------------------
 * 名称：update-requests.ts
 * 说明：system.updateStatus、system.checkUpdate、system.installUpdate 的处理：读取升级状态、开始检查更新、重启并安装已经下载好的新版本。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：升级服务器的地址与安装方式由升级服务决定，请求都没有参数；检查只负责开始，进展经状态推送给界面（见 forward-update-status.ts）。
 * ------------------------------------------------------------------------
 */

import type { MainResult } from '../../../contracts/core-gateway/generated/core-api';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import type { AppUpdater } from '../../../contracts/system/app-updater';
import type { MainRequestHandler } from '../core/main-request-ipc';

/** 错误码。 */
const UPDATE_NOT_READY = 'system.updateNotReady';

/** 用户可见的文字。 */
const TEXT = {
  notReady: '还没有下载好的新版本'
} as const;

/** 升级请求处理所需的依赖。 */
export interface UpdateRequestOptions {
  readonly updater: Pick<AppUpdater, 'status' | 'check' | 'install'>;
}

/** 创建 system.updateStatus 的处理函数。 */
export function createUpdateStatusHandler(options: UpdateRequestOptions): MainRequestHandler {
  return (): Promise<MainResult<'system.updateStatus'>> => Promise.resolve({ status: options.updater.status() });
}

/** 创建 system.checkUpdate 的处理函数。 */
export function createCheckUpdateHandler(options: UpdateRequestOptions): MainRequestHandler {
  return (): Promise<MainResult<'system.checkUpdate'>> => Promise.resolve({ status: options.updater.check() });
}

/** 创建 system.installUpdate 的处理函数：没有下载好的新版本时返回 system.updateNotReady。 */
export function createInstallUpdateHandler(options: UpdateRequestOptions): MainRequestHandler {
  return (): Promise<MainResult<'system.installUpdate'>> => {
    if (!options.updater.install()) return Promise.reject(new CoreRequestError({ code: UPDATE_NOT_READY, message: TEXT.notReady }));
    return Promise.resolve({});
  };
}
