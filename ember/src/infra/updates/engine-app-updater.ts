/*
 * ------------------------------------------------------------------------
 * 名称：engine-app-updater.ts
 * 说明：应用升级服务的实现：用升级引擎检查与下载新版本，维护升级状态（不支持、空闲、检查中、下载中、已下载、已是最新、失败）并通知订阅者。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：引擎由组装根注入；检查阶段有超时（引擎偶尔不发任何事件，不超时的话状态会一直停在“检查中”，再也不能重新检查），下载阶段不设超时，失败由引擎报告；
 *       已经下载好（ready）之后不再接受引擎的事件，也不再检查。
 * ------------------------------------------------------------------------
 */

import type { AppUpdater, UpdateEngine } from '../../contracts/system/app-updater';
import type { UpdateStatus } from '../../contracts/system/generated/system';

/** 检查阶段默认的超时（毫秒）。 */
const DEFAULT_CHECK_TIMEOUT_MS = 60_000;

/** 用户可见的文字。 */
const TEXT = {
  checkTimeout: '检查更新超时，请稍后再试',
  startFailed: (reason: string) => `无法开始检查更新：${reason}`
} as const;

/** 升级服务的依赖。 */
export interface EngineAppUpdaterOptions {
  readonly engine: UpdateEngine;
  /** 检查阶段的超时（毫秒），默认 60 秒。 */
  readonly checkTimeoutMs?: number;
}

/** 异常里的说明文字。 */
function reasonOf(error: unknown): string {
  return error instanceof Error && error.message !== '' ? error.message : String(error);
}

/** 基于升级引擎的应用升级服务。 */
export class EngineAppUpdater implements AppUpdater {
  private current: UpdateStatus;
  private readonly listeners = new Set<(status: UpdateStatus) => void>();
  private checkTimer: ReturnType<typeof setTimeout> | undefined;
  private readonly checkTimeoutMs: number;

  constructor(private readonly options: EngineAppUpdaterOptions) {
    this.checkTimeoutMs = options.checkTimeoutMs ?? DEFAULT_CHECK_TIMEOUT_MS;
    this.current = { state: options.engine.supported ? 'idle' : 'unsupported' };
    if (options.engine.supported) {
      options.engine.subscribe({
        available: () => {
          if (this.current.state === 'checking') this.update({ state: 'downloading' });
        },
        notAvailable: () => {
          if (this.current.state === 'checking') this.update({ state: 'upToDate' });
        },
        downloaded: (version) => {
          if (this.current.state === 'checking' || this.current.state === 'downloading') this.update(version === undefined ? { state: 'ready' } : { state: 'ready', version });
        },
        failed: (message) => {
          if (this.current.state === 'checking' || this.current.state === 'downloading') this.update({ state: 'error', message });
        }
      });
    }
  }

  status(): UpdateStatus {
    return this.current;
  }

  check(): UpdateStatus {
    if (this.current.state !== 'idle' && this.current.state !== 'upToDate' && this.current.state !== 'error') return this.current;
    this.update({ state: 'checking' });
    this.checkTimer = setTimeout(() => {
      if (this.current.state === 'checking') this.update({ state: 'error', message: TEXT.checkTimeout });
    }, this.checkTimeoutMs);
    try {
      this.options.engine.check();
    } catch (error) {
      this.update({ state: 'error', message: TEXT.startFailed(reasonOf(error)) });
    }
    return this.current;
  }

  install(): boolean {
    if (this.current.state !== 'ready') return false;
    this.options.engine.restart();
    return true;
  }

  onStatusChange(listener: (status: UpdateStatus) => void): () => void {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  }

  /** 换成新状态并通知订阅者；离开检查阶段时撤掉超时。 */
  private update(next: UpdateStatus): void {
    if (next.state !== 'checking' && this.checkTimer !== undefined) {
      clearTimeout(this.checkTimer);
      this.checkTimer = undefined;
    }
    this.current = next;
    for (const listener of this.listeners) listener(next);
  }
}
