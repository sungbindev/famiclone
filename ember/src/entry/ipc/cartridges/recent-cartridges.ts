/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges.ts
 * 说明：cartridges.recent 与 cartridges.openRecent 的处理：读出最近打开的清单（只给显示名与编号），按编号重新打开卡带。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只在主进程里，渲染进程只能传编号；编号不在清单里时返回 cartridges.recentNotFound；加载失败的卡带由 loadAndRemember 移出清单。
 * ------------------------------------------------------------------------
 */

import type { MainParams, MainResult } from '../../../contracts/core-gateway/generated/core-api';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import type { MainRequestHandler } from '../core/main-request-ipc';
import { loadAndRemember, type LoadCartridgeOptions } from './load-cartridge';

/** 用户可见的文字。 */
const TEXT = {
  notFound: '这个卡带已不在最近打开的清单里'
} as const;

/** 清单里找不到编号时的错误码。 */
const RECENT_NOT_FOUND = 'cartridges.recentNotFound';

/**
 * 创建 cartridges.recent 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createRecentCartridgesHandler(options: LoadCartridgeOptions): MainRequestHandler {
  return (): Promise<MainResult<'cartridges.recent'>> => Promise.resolve({ items: options.recent.list().map((item) => ({ id: item.id, name: item.name })) });
}

/**
 * 创建 cartridges.openRecent 的处理函数。
 * @param options 依赖。
 * @returns 处理函数。
 */
export function createOpenRecentCartridgeHandler(options: LoadCartridgeOptions): MainRequestHandler {
  return async (params): Promise<MainResult<'cartridges.openRecent'>> => {
    const { id } = params as MainParams<'cartridges.openRecent'>;
    const path = options.recent.resolve(id);
    if (path === undefined) throw new CoreRequestError({ code: RECENT_NOT_FOUND, message: TEXT.notFound });
    return { loaded: await loadAndRemember(options, path) };
  };
}
