/*
 * ------------------------------------------------------------------------
 * 名称：shell-bridge.ts
 * 说明：外壳与主进程之间的约定：IPC 通道名、核心请求的返回格式，以及预加载脚本暴露给界面的 window.ember 的类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：界面只经 window.ember 与主进程通信；核心请求按 Schema 生成的 CoreApi 暴露为具名方法，没有通用的调用入口；
 *       主进程自己处理的请求（如打开卡带的文件选择）按生成的 MainApi 暴露，画面与输入的端口经 streams 取得；
 *       外壳自己的能力（界面状态的保存、窗口动作、键盘独占）经 shell 暴露。
 * ------------------------------------------------------------------------
 */

import type { CoreErrorInfo, CoreStatus } from '../core-gateway/core-messages';
import type { CoreApi, MainApi } from '../core-gateway/generated/core-api';
import type { StreamPortsBridge } from '../stream-ports/stream-ports';
import type { NativeKeyEvent } from './keyboard-capture';
import type { ShellLayout, ShellState } from './shell-state-service';
import type { WindowAction, WindowState } from './window-control';

/** IPC 通道名。 */
export const SHELL_CHANNELS = {
  coreRequest: 'core:request',
  coreEvent: 'core:event',
  coreStatus: 'core:status',
  coreStatusChanged: 'core:status-changed',
  mainRequest: 'main:request',
  getState: 'shell:get-state',
  setLayout: 'shell:set-layout',
  windowAction: 'shell:window-action',
  getWindowState: 'shell:get-window-state',
  windowStateChanged: 'shell:window-state-changed',
  setKeyboardCapture: 'shell:set-keyboard-capture',
  nativeKey: 'shell:native-key',
  keyboardReleased: 'shell:keyboard-released'
} as const;

/** 核心请求的 IPC 返回：成功时为结果，失败时为错误码与说明。 */
export type CoreRequestReply = { readonly ok: true; readonly result: unknown } | { readonly ok: false; readonly error: CoreErrorInfo };

/** 外壳自己的能力：界面状态的保存、窗口动作与键盘独占。 */
export interface ShellBridge {
  /** 读取保存的界面状态（窗口位置之外，界面只用布局）。 */
  getState(): Promise<ShellState>;
  /** 保存布局的修改；不合格的修改会被主进程丢弃。 */
  setLayout(patch: Partial<ShellLayout>): void;
  /** 发起窗口动作；不在白名单里的动作会被主进程丢弃。 */
  windowAction(action: WindowAction): void;
  /** 窗口当前的状态。 */
  getWindowState(): Promise<WindowState>;
  /** 订阅窗口状态的变化；返回取消订阅的函数。 */
  onWindowStateChange(listener: (state: WindowState) => void): () => void;
  /** 开启或关闭键盘独占；返回调用后独占是否生效（不支持独占的平台为 false，界面继续用页面内的键盘事件）。 */
  setKeyboardCapture(enabled: boolean): Promise<boolean>;
  /** 订阅独占期间的物理按键；返回取消订阅的函数。 */
  onNativeKey(listener: (event: NativeKeyEvent) => void): () => void;
  /** 订阅“独占被主进程自动关闭”（窗口失去焦点、隐藏、最小化或关闭）；返回取消订阅的函数。 */
  onKeyboardReleased(listener: () => void): () => void;
}

/** 预加载脚本暴露给界面的接口（window.ember）。 */
export interface EmberBridge {
  /** 核心请求，按模块分组的具名方法；失败时以 CoreErrorInfo（{code, message}）拒绝。 */
  readonly core: CoreApi;
  /** 主进程自己处理的请求，按模块分组的具名方法（如 cartridges.open 弹出文件选择窗口）；失败时同样以 CoreErrorInfo 拒绝。 */
  readonly main: MainApi;
  /** 画面与输入的消息端口。 */
  readonly streams: StreamPortsBridge;
  /** 外壳自己的能力。 */
  readonly shell: ShellBridge;
  /** 订阅核心事件；返回取消订阅的函数。 */
  onCoreEvent(listener: (name: string, data: unknown) => void): () => void;
  /** 核心的运行状态。 */
  getCoreStatus(): Promise<CoreStatus>;
  /** 订阅核心运行状态的变化（核心进程重启时依次为启动中、就绪）；返回取消订阅的函数。 */
  onCoreStatusChange(listener: (status: CoreStatus) => void): () => void;
}
