/*
 * ------------------------------------------------------------------------
 * 名称：update-controller.ts
 * 说明：应用升级在界面里的控制器（不依赖 React）：保存主进程推送的升级状态，决定“帮助”菜单里检查更新项的名称与可用状态，执行检查与重启安装，并在合适的时候用通知告知用户。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：自动检查（启动时）只在新版本下载好时通知，用户手动检查才通知“已是最新版本”“检查失败”等结果；状态由主进程维护，这里只读取、订阅与发起请求。
 * ------------------------------------------------------------------------
 */

import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { UpdateStatus } from '../../contracts/system/generated/system';
import type { ShellApi } from '../shell/shell-registry';

/** 控制器用到的预加载接口。 */
export type UpdateBridge = Pick<EmberBridge, 'main' | 'onUpdateStatusChange'>;

/** 控制器需要外壳提供的能力。 */
export interface UpdateHost {
  readonly notify: ShellApi['notify'];
  /** 状态变化后让外壳重新计算菜单。 */
  readonly refresh: () => void;
}

/** 用户可见的文字。 */
const TEXT = {
  check: '检查更新…',
  checking: '正在检查更新…',
  downloading: '正在下载更新…',
  install: '重启并安装更新',
  installVersion: (version: string) => `重启并安装更新（${version}）`,
  checkStarted: '正在检查更新…',
  unsupported: '当前运行方式不支持检查更新，请使用安装版。',
  found: '发现新版本，正在下载。',
  upToDate: '已是最新版本。',
  ready: (version: string | undefined) => `${version === undefined ? '新版本' : `新版本 ${version} `}已下载好，在“帮助”菜单里选择“重启并安装更新”即可使用。`,
  checkFailed: '检查更新失败',
  installFailed: '无法重启安装更新'
} as const;

/** 错误里给用户看的说明；错误没有说明时用 fallback。 */
function errorText(error: unknown, fallback: string): string {
  if (typeof error === 'object' && error !== null && 'message' in error && typeof error.message === 'string' && error.message !== '') return error.message;
  return fallback;
}

/** 升级控制器。 */
export class UpdateController {
  private status: UpdateStatus = { state: 'idle' };
  /** 当前这次检查是不是用户手动发起的。 */
  private manual = false;
  private attachment: { readonly bridge: UpdateBridge; readonly host: UpdateHost } | undefined;

  /**
   * 接上预加载接口与外壳：读取当前状态并订阅变化。
   * @returns 断开的函数。
   */
  attach(bridge: UpdateBridge, host: UpdateHost): () => void {
    const attachment = { bridge, host };
    this.attachment = attachment;
    const stop = bridge.onUpdateStatusChange((status) => {
      if (this.attachment === attachment) this.apply(status, true);
    });
    bridge.main.system.updateStatus({}).then(
      (result) => {
        if (this.attachment === attachment) this.apply(result.status, false);
      },
      () => undefined
    );
    return () => {
      stop();
      if (this.attachment === attachment) this.attachment = undefined;
    };
  }

  /** 菜单项当前显示的名称。 */
  title(): string {
    switch (this.status.state) {
      case 'checking':
        return TEXT.checking;
      case 'downloading':
        return TEXT.downloading;
      case 'ready':
        return this.status.version === undefined ? TEXT.install : TEXT.installVersion(this.status.version);
      default:
        return TEXT.check;
    }
  }

  /** 菜单项当前是否可用：检查与下载过程中不能再操作。 */
  canRun(): boolean {
    return this.status.state !== 'checking' && this.status.state !== 'downloading';
  }

  /**
   * 执行菜单项：新版本已下载好时重启安装，否则手动检查更新。
   * @param shell 外壳接口，失败与结果用它的通知提示。
   */
  run(shell: ShellApi): void {
    const bridge = this.attachment?.bridge;
    if (bridge === undefined || !this.canRun()) return;
    if (this.status.state === 'ready') {
      bridge.main.system.installUpdate({}).catch((error: unknown) => shell.notify(errorText(error, TEXT.installFailed), 'error'));
      return;
    }
    this.manual = true;
    bridge.main.system.checkUpdate({}).then(
      (result) => {
        // 完成、失败等结果由状态推送告知；这里只处理请求返回时就已经确定的两种情况。
        if (result.status.state === 'unsupported') {
          this.manual = false;
          shell.notify(TEXT.unsupported, 'info');
        } else if (result.status.state === 'checking') {
          shell.notify(TEXT.checkStarted, 'info');
        }
      },
      (error: unknown) => {
        this.manual = false;
        shell.notify(errorText(error, TEXT.checkFailed), 'error');
      }
    );
  }

  /** 记下新状态，让外壳重算菜单，并按需要通知用户；announce 为 false 时只记录不通知（读取初始状态）。 */
  private apply(next: UpdateStatus, announce: boolean): void {
    this.status = next;
    const host = this.attachment?.host;
    host?.refresh();
    if (!announce || host === undefined) return;
    switch (next.state) {
      case 'ready':
        this.manual = false;
        host.notify(TEXT.ready(next.version), 'info');
        break;
      case 'downloading':
        if (this.manual) host.notify(TEXT.found, 'info');
        break;
      case 'upToDate':
        if (this.manual) host.notify(TEXT.upToDate, 'info');
        this.manual = false;
        break;
      case 'error':
        if (this.manual) host.notify(next.message === undefined ? TEXT.checkFailed : `${TEXT.checkFailed}：${next.message}`, 'error');
        this.manual = false;
        break;
      default:
        break;
    }
  }
}
