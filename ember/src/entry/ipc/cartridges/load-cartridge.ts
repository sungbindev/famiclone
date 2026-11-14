/*
 * ------------------------------------------------------------------------
 * 名称：load-cartridge.ts
 * 说明：主进程加载卡带并记入最近打开：先把路径记入清单，再请核心 cartridges.load，加载失败时把它移出清单后原样抛出错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：先记入再加载，是为了核心发出 cartridges.loaded 事件、界面重新读取清单时清单已经包含这个卡带；
 *       cartridges.open、cartridges.openRecent 与启动参数 --cartridge= 都经这里加载。
 * ------------------------------------------------------------------------
 */

import type { RecentCartridgesService } from '../../../contracts/cartridges/recent-cartridges-service';
import { requestCoreAsMain, type CoreRequestHandling } from '../core/core-ipc';

/** 加载卡带所需的依赖。 */
export interface LoadCartridgeOptions {
  readonly core: CoreRequestHandling;
  readonly recent: RecentCartridgesService;
}

/**
 * 加载卡带并记入最近打开。
 * @param options 依赖。
 * @param path 卡带路径。
 * @returns cartridges.load 的结果。
 * @throws CoreRequestError 加载失败时，带核心给出的错误码与说明。
 */
export async function loadAndRemember(options: LoadCartridgeOptions, path: string) {
  options.recent.add(path);
  try {
    return await requestCoreAsMain(options.core, 'cartridges.load', { path });
  } catch (error) {
    options.recent.remove(path);
    throw error;
  }
}
