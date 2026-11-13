/*
 * ------------------------------------------------------------------------
 * 名称：preload.ts
 * 说明：预加载脚本：向界面暴露 window.ember，界面可发起的核心请求按 Schema 生成的请求名暴露为“模块.操作”的具名方法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：不暴露 ipcRenderer 与通用的调用入口；回调只收到数据，不收到 IPC 事件对象；核心请求失败时以 {code, message} 拒绝。
 * ------------------------------------------------------------------------
 */

import { contextBridge, ipcRenderer, type IpcRendererEvent } from 'electron';
import { CORE_REQUEST_NAMES, type CoreApi } from '../../contracts/core-gateway/generated/core-api';
import { SHELL_CHANNELS, type CoreRequestReply, type EmberBridge } from '../../contracts/shell/shell-bridge';

/** 订阅主进程推送的消息，返回取消订阅的函数。 */
function subscribe<T extends unknown[]>(channel: string, listener: (...args: T) => void): () => void {
  const handler = (_event: IpcRendererEvent, ...args: unknown[]): void => listener(...(args as T));
  ipcRenderer.on(channel, handler);
  return () => {
    ipcRenderer.removeListener(channel, handler);
  };
}

/** 发送一次核心请求。 */
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

const bridge: EmberBridge = {
  core: buildApi(CORE_REQUEST_NAMES, SHELL_CHANNELS.coreRequest) as unknown as CoreApi,
  onCoreEvent: (listener) => subscribe(SHELL_CHANNELS.coreEvent, listener),
  getCoreStatus: () => ipcRenderer.invoke(SHELL_CHANNELS.coreStatus)
};

contextBridge.exposeInMainWorld('ember', bridge);
