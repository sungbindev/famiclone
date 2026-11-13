/*
 * ------------------------------------------------------------------------
 * 名称：preload.ts
 * 说明：预加载脚本：向界面暴露 window.ember，界面可发起的核心请求与主进程请求按 Schema 生成的请求名暴露为“模块.操作”的具名方法，另有接收画面与输入端口的方法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：不暴露 ipcRenderer 与通用的调用入口；回调只收到数据，不收到 IPC 事件对象；核心请求失败时以 {code, message} 拒绝。
 *       端口：页面调用 streams.requestPorts() 后，主进程把两个 MessagePort 发给本脚本；本脚本确认标记与端口数量后，
 *       用 window.postMessage 把端口交给页面（上下文隔离下 contextBridge 无法传递端口），页面须检查 event.source 是自己的窗口。
 * ------------------------------------------------------------------------
 */

import { contextBridge, ipcRenderer, type IpcRendererEvent } from 'electron';
import { CORE_REQUEST_NAMES, MAIN_REQUEST_NAMES, type CoreApi, type MainApi } from '../../contracts/core-gateway/generated/core-api';
import { SHELL_CHANNELS, type CoreRequestReply, type EmberBridge } from '../../contracts/shell/shell-bridge';
import { STREAM_PORTS_WINDOW_MESSAGE, type StreamPortsWindowMessage } from '../../contracts/stream-ports/stream-messages';
import { STREAM_CHANNELS, type StreamPortsBridge } from '../../contracts/stream-ports/stream-ports';

/** 主进程交给页面的端口个数：画面端口与输入端口。 */
const STREAM_PORT_COUNT = 2;

/** 订阅主进程推送的消息，返回取消订阅的函数。 */
function subscribe<T extends unknown[]>(channel: string, listener: (...args: T) => void): () => void {
  const handler = (_event: IpcRendererEvent, ...args: unknown[]): void => listener(...(args as T));
  ipcRenderer.on(channel, handler);
  return () => {
    ipcRenderer.removeListener(channel, handler);
  };
}

/** 发送一次请求。 */
async function request(channel: string, name: string, params: unknown): Promise<unknown> {
  const reply = (await ipcRenderer.invoke(channel, name, params)) as CoreRequestReply;
  if (reply.ok) return reply.result;
  // Error 经 contextBridge 复制时会丢失 code 等自定义属性，所以以普通对象拒绝。
  throw { code: reply.error.code, message: reply.error.message };
}

/** 按请求名生成按模块分组的具名方法。 */
function buildApi(names: readonly string[], channel: string): Record<string, Record<string, (params: unknown) => Promise<unknown>>> {
  const api: Record<string, Record<string, (params: unknown) => Promise<unknown>>> = {};
  for (const name of names) {
    const dot = name.indexOf('.');
    const module = name.slice(0, dot);
    const operation = name.slice(dot + 1);
    api[module] ??= {};
    api[module][operation] = (params) => request(channel, name, params);
  }
  return api;
}

/** 主进程发来端口：标记与数量都对才交给页面，否则丢弃。 */
ipcRenderer.on(STREAM_CHANNELS.ports, (event: IpcRendererEvent, tag: unknown) => {
  if (tag !== STREAM_PORTS_WINDOW_MESSAGE || event.ports.length !== STREAM_PORT_COUNT) return;
  const message: StreamPortsWindowMessage = { type: STREAM_PORTS_WINDOW_MESSAGE };
  // file 页面的源是 "null"，不能作为目标源；这时消息只发给本窗口自己，用 "*" 即可。
  const targetOrigin = window.location.origin === 'null' ? '*' : window.location.origin;
  window.postMessage(message, targetOrigin, [...event.ports]);
});

const streams: StreamPortsBridge = {
  requestPorts: () => ipcRenderer.send(STREAM_CHANNELS.connect)
};

const bridge: EmberBridge = {
  core: buildApi(CORE_REQUEST_NAMES, SHELL_CHANNELS.coreRequest) as unknown as CoreApi,
  main: buildApi(MAIN_REQUEST_NAMES, SHELL_CHANNELS.mainRequest) as unknown as MainApi,
  streams,
  onCoreEvent: (listener) => subscribe(SHELL_CHANNELS.coreEvent, listener),
  getCoreStatus: () => ipcRenderer.invoke(SHELL_CHANNELS.coreStatus)
};

contextBridge.exposeInMainWorld('ember', bridge);
