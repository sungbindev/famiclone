/*
 * ------------------------------------------------------------------------
 * 名称：theme-sync.ts
 * 说明：主题同步：核心就绪后读取首选项里的主题，之后每次变化都交给系统主题来源，界面的主题与窗口底色随系统主题变化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：主题设置项是 appearance.theme，取值 system（跟随系统）、dark、light；取值不认识时不改动。
 *       first 在第一次读到并应用主题（或读取失败）时结束，主窗口创建时等它再显示，启动时就不会先闪一下错误的颜色；
 *       核心进程重启后重新读取。真正设置系统主题来源的函数由组装根注入，本文件不引用 electron。
 * ------------------------------------------------------------------------
 */

import type { CoreGateway } from '../../contracts/core-gateway/core-gateway';

/** 主题来源。 */
export type ThemeSource = 'system' | 'dark' | 'light';

/** 主题设置项的标识。 */
export const THEME_SETTING_ID = 'appearance.theme';

/** 全部合格的主题来源。 */
const THEME_SOURCES: readonly string[] = ['system', 'dark', 'light'];

/** 主题同步的依赖。 */
export interface ThemeSyncOptions {
  readonly gateway: CoreGateway;
  /** 设置系统的主题来源。 */
  readonly applyThemeSource: (source: ThemeSource) => void;
  /** 读取首选项失败时调用，不影响运行。 */
  readonly onError?: (error: unknown) => void;
}

/** 一次主题同步。 */
export interface ThemeSync {
  /** 第一次读到并应用主题（或读取失败）之后结束，不会被拒绝。 */
  readonly first: Promise<void>;
  /** 停止同步。 */
  stop(): void;
}

function isThemeSource(value: unknown): value is ThemeSource {
  return typeof value === 'string' && THEME_SOURCES.includes(value);
}

/** 一个可以从外面结束的 Promise。 */
function createDeferred(): { promise: Promise<void>; resolve: () => void } {
  let resolver: (() => void) | undefined;
  const promise = new Promise<void>((resolvePromise) => {
    resolver = resolvePromise;
  });
  return { promise, resolve: () => resolver?.() };
}

/**
 * 开始同步主题。
 * @param options 依赖。
 */
export function startThemeSync(options: ThemeSyncOptions): ThemeSync {
  let stopped = false;
  const first = createDeferred();

  const apply = (value: unknown): void => {
    if (!stopped && isThemeSource(value)) options.applyThemeSource(value);
  };
  const refresh = (): void => {
    options.gateway
      .request('preferences.list', {})
      .then((result) => apply(result.settings.find((setting) => setting.id === THEME_SETTING_ID)?.value))
      .catch((error: unknown) => options.onError?.(error))
      .finally(first.resolve);
  };

  const stopEvents = options.gateway.onEvent((name, data) => {
    if (name !== 'preferences.changed' || typeof data !== 'object' || data === null) return;
    const change = data as { id?: unknown; value?: unknown };
    if (change.id === THEME_SETTING_ID) apply(change.value);
  });
  const stopStatus = options.gateway.onStatusChange((status) => {
    if (status.state === 'ready') refresh();
  });
  if (options.gateway.status().state === 'ready') refresh();

  return {
    first: first.promise,
    stop: () => {
      stopped = true;
      stopEvents();
      stopStatus();
      first.resolve();
    }
  };
}
