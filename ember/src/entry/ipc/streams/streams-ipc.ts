/*
 * ------------------------------------------------------------------------
 * 名称：streams-ipc.ts
 * 说明：画面与输入端口请求的 IPC 入口：页面请求端口时，检查发送方是应用自己的页面、是主窗口的主框架，再交给端口集线器建立端口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只有主窗口的主框架能收到端口；请求没有参数，端口经 webContents.postMessage 交给请求的窗口，不经返回值。
 * ------------------------------------------------------------------------
 */

import type { IpcMain, IpcMainEvent, WebContents } from 'electron';
import { STREAM_CHANNELS, type StreamConnector, type StreamTarget } from '../../../contracts/stream-ports/stream-ports';
import type { SenderCheck } from '../sender-check';

/** 端口请求 IPC 的依赖。 */
export interface StreamsIpcOptions {
  readonly ipc: IpcMain;
  readonly connector: StreamConnector;
  readonly isTrustedSender: SenderCheck;
  /** 判断页面是不是有资格收到端口的窗口（主窗口）。 */
  readonly isStreamWindow: (contents: WebContents) => boolean;
  /** 把页面包装成端口的接收方。 */
  readonly createTarget: (contents: WebContents) => StreamTarget;
}

/** 处理一次端口请求；导出供测试使用。 */
export function handleStreamsConnect(options: Omit<StreamsIpcOptions, 'ipc'>, event: IpcMainEvent): void {
  if (!options.isTrustedSender(event)) return;
  const contents = event.sender;
  if (contents.isDestroyed() || !options.isStreamWindow(contents) || event.senderFrame !== contents.mainFrame) return;
  options.connector.connect(options.createTarget(contents));
}

/** 登记端口请求的 IPC 处理。 */
export function registerStreamsIpc(options: StreamsIpcOptions): void {
  options.ipc.on(STREAM_CHANNELS.connect, (event) => handleStreamsConnect(options, event));
}
