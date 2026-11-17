/*
 * ------------------------------------------------------------------------
 * 名称：app-updater.ts
 * 说明：应用升级的抽象：AppUpdater 是主进程对外提供的升级服务（读取状态、开始检查、重启安装、订阅状态变化），UpdateEngine 是它背后的升级引擎（Windows 上是 Squirrel）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：升级服务器的地址只在引擎的实现里，界面不能指定；状态的类型由 Schema 生成（system 模块的 UpdateStatus）。
 * ------------------------------------------------------------------------
 */

import type { UpdateStatus } from './generated/system';

/** 升级引擎发出的事件。 */
export interface UpdateEngineEvents {
  /** 有新版本，引擎开始下载。 */
  available(): void;
  /** 已是最新版本。 */
  notAvailable(): void;
  /** 新版本已下载好；version 是新版本的版本号，引擎拿不到时为 undefined。 */
  downloaded(version: string | undefined): void;
  /** 检查或下载失败。 */
  failed(message: string): void;
}

/** 升级引擎：真正去升级服务器检查、下载并安装新版本。 */
export interface UpdateEngine {
  /** 当前运行方式是否支持升级（开发版、免安装目录、不支持的系统都是 false）。 */
  readonly supported: boolean;
  /** 订阅引擎的事件；返回取消订阅的函数。 */
  subscribe(events: UpdateEngineEvents): () => void;
  /** 开始检查；有新版本时引擎自动下载。无法开始时抛出异常。 */
  check(): void;
  /** 退出应用并以新版本重新启动。 */
  restart(): void;
}

/** 应用升级服务。 */
export interface AppUpdater {
  /** 当前状态。 */
  status(): UpdateStatus;
  /** 开始检查更新并返回当前状态；正在检查、正在下载、已经下载好或不支持升级时不重复检查。 */
  check(): UpdateStatus;
  /** 重启并安装已下载好的新版本；状态不是 ready 时什么也不做并返回 false。 */
  install(): boolean;
  /** 订阅状态变化；返回取消订阅的函数。 */
  onStatusChange(listener: (status: UpdateStatus) => void): () => void;
}
