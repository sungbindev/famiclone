/*
 * ------------------------------------------------------------------------
 * 名称：squirrel-update-engine.ts
 * 说明：升级引擎的 Windows 实现：用 Electron 的 autoUpdater（Squirrel.Windows）到升级地址检查更新、下载新版本，下载好后经安装根目录的启动器重新启动。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：升级地址是一个静态目录，里面放 RELEASES 与 .nupkg；有新版本时 Squirrel 在下载时就把它安装到新的 app-<版本> 目录，下次从启动器启动就是新版本，所以重启只是用启动器重新启动，不调用 quitAndInstall：
 *       应用要先走自己的退出流程（保存界面状态、停止核心进程），重启交给 Electron 在进程退出时执行，不会与旧进程抢单实例锁。
 *       启动器为 undefined（不是 Squirrel 安装的应用）时引擎不支持升级，不碰 autoUpdater。
 *       Squirrel 的报错是系统代码页的文字（中文系统上按 UTF-8 显示是乱码），不能直接给用户看：界面只收到一句固定说明，原始报错交给 onDiagnostic 写日志。
 * ------------------------------------------------------------------------
 */

import { app, autoUpdater } from 'electron';

/** 引擎事件的订阅者，与契约里的 UpdateEngineEvents 结构一致（platform 不引用 contracts）。 */
export interface SquirrelEngineEvents {
  available(): void;
  notAvailable(): void;
  downloaded(version: string | undefined): void;
  failed(message: string): void;
}

/** 引擎，与契约里的 UpdateEngine 结构一致。 */
export interface SquirrelUpdateEngine {
  readonly supported: boolean;
  subscribe(events: SquirrelEngineEvents): () => void;
  check(): void;
  restart(): void;
}

/** 引擎的配置。 */
export interface SquirrelEngineOptions {
  /** 升级地址：存放 RELEASES 与 .nupkg 的目录。 */
  readonly feedUrl: string;
  /** 安装根目录里的启动器；不是 Squirrel 安装的应用时为 undefined。 */
  readonly launcher: string | undefined;
  /** Squirrel 报告失败时收到它的原始报错，用来写日志；不给时丢弃。 */
  readonly onDiagnostic?: (error: Error) => void;
}

/** 版本号的合格写法：数字开头，只含字母、数字与 . + -。 */
const VERSION_PATTERN = /^[0-9][0-9A-Za-z.+-]{0,63}$/;

/** 用户可见的文字。 */
const TEXT = {
  failed: '无法从升级服务器取得更新，详细原因已写入日志（“帮助→打开日志目录”）。',
  notSupported: '当前运行方式不支持升级'
} as const;

/** 从 Squirrel 给出的发布名里取版本号；不是合格的版本号时返回 undefined。 */
function versionOf(releaseName: unknown): string | undefined {
  if (typeof releaseName !== 'string') return undefined;
  const text = releaseName.trim();
  return VERSION_PATTERN.test(text) ? text : undefined;
}

/**
 * 创建 Windows 的升级引擎。
 * @param options 配置。
 */
export function createSquirrelUpdateEngine(options: SquirrelEngineOptions): SquirrelUpdateEngine {
  const { launcher } = options;
  const subscribers = new Set<SquirrelEngineEvents>();
  const emit = (notify: (events: SquirrelEngineEvents) => void): void => {
    for (const events of subscribers) notify(events);
  };

  if (launcher !== undefined) {
    autoUpdater.setFeedURL({ url: options.feedUrl });
    autoUpdater.on('update-available', () => emit((events) => events.available()));
    autoUpdater.on('update-not-available', () => emit((events) => events.notAvailable()));
    autoUpdater.on('update-downloaded', (_event, _releaseNotes, releaseName) => emit((events) => events.downloaded(versionOf(releaseName))));
    autoUpdater.on('error', (error) => {
      options.onDiagnostic?.(error);
      emit((events) => events.failed(TEXT.failed));
    });
  }

  return {
    supported: launcher !== undefined,
    subscribe: (events) => {
      subscribers.add(events);
      return () => {
        subscribers.delete(events);
      };
    },
    check: () => {
      if (launcher === undefined) throw new Error(TEXT.notSupported);
      autoUpdater.checkForUpdates();
    },
    restart: () => {
      if (launcher === undefined) throw new Error(TEXT.notSupported);
      // 重启在进程退出时才执行；先放掉单实例锁，新进程起来时不会被当作第二个实例。
      app.releaseSingleInstanceLock();
      app.relaunch({ execPath: launcher, args: [] });
      app.quit();
    }
  };
}
