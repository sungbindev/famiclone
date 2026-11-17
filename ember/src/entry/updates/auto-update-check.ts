/*
 * ------------------------------------------------------------------------
 * 名称：auto-update-check.ts
 * 说明：启动时自动检查更新：核心就绪后读取首选项 update.autoCheck，开启时等一小会儿再检查一次；之后设置项变化会立即生效（检查前关闭就取消，检查前才开启就安排检查）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：每次运行只自动检查一次，之后只有用户在“帮助”菜单里手动检查；不支持升级的运行方式不做任何事；
 *       延迟是为了不与启动抢资源，也避开 Squirrel 首次启动时锁住文件的几秒钟（首次启动由组装根直接不启动本检查）。核心进程重启后会重新读取设置项。
 * ------------------------------------------------------------------------
 */

import type { CoreGateway } from '../../contracts/core-gateway/core-gateway';
import type { AppUpdater } from '../../contracts/system/app-updater';

/** 自动检查更新设置项的标识。 */
export const AUTO_CHECK_SETTING_ID = 'update.autoCheck';

/** 自动检查的依赖。 */
export interface AutoUpdateCheckOptions {
  readonly gateway: CoreGateway;
  readonly updater: Pick<AppUpdater, 'status' | 'check'>;
  /** 从开启自动检查到真正检查之间等待的毫秒数。 */
  readonly delayMs: number;
  /** 读取首选项失败时调用，不影响运行。 */
  readonly onError?: (error: unknown) => void;
}

/** 一次自动检查。 */
export interface AutoUpdateCheck {
  /** 停止：取消还没到时间的检查，之后不再响应设置项的变化。 */
  stop(): void;
}

/**
 * 开始自动检查更新。
 * @param options 依赖。
 */
export function startAutoUpdateCheck(options: AutoUpdateCheckOptions): AutoUpdateCheck {
  if (options.updater.status().state === 'unsupported') return { stop: () => undefined };

  /** 已经检查过，或已经停止。 */
  let finished = false;
  let timer: ReturnType<typeof setTimeout> | undefined;
  const cancel = (): void => {
    if (timer !== undefined) clearTimeout(timer);
    timer = undefined;
  };
  const apply = (value: unknown): void => {
    if (finished) return;
    if (value === false) cancel();
    if (value !== true || timer !== undefined) return;
    timer = setTimeout(() => {
      timer = undefined;
      finished = true;
      options.updater.check();
    }, options.delayMs);
  };
  const refresh = (): void => {
    options.gateway
      .request('preferences.list', {})
      .then((result) => apply(result.settings.find((setting) => setting.id === AUTO_CHECK_SETTING_ID)?.value))
      .catch((error: unknown) => options.onError?.(error));
  };

  const stopEvents = options.gateway.onEvent((name, data) => {
    if (name !== 'preferences.changed' || typeof data !== 'object' || data === null) return;
    const change = data as { id?: unknown; value?: unknown };
    if (change.id === AUTO_CHECK_SETTING_ID) apply(change.value);
  });
  const stopStatus = options.gateway.onStatusChange((status) => {
    if (status.state === 'ready') refresh();
  });
  if (options.gateway.status().state === 'ready') refresh();

  return {
    stop: () => {
      finished = true;
      cancel();
      stopEvents();
      stopStatus();
    }
  };
}
