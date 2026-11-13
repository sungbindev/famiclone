/*
 * ------------------------------------------------------------------------
 * 名称：electron-stream-ports.ts
 * 说明：消息端口的 Electron 实现：用 MessageChannelMain 创建通道，把端口经 webContents.postMessage 交给窗口的页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：platform 层不引用 contracts，返回值按结构与契约接口一致，由组装根注入；通道名与窗口消息的类型由调用方传入；
 *       端口只交给指定的窗口，预加载脚本收到后经窗口消息转给页面，见 entry/preload/preload.ts。
 * ------------------------------------------------------------------------
 */

import { MessageChannelMain, type MessagePortMain, type WebContents } from 'electron';

/** 一条通道的两端。 */
export interface ElectronStreamChannel {
  readonly first: object;
  readonly second: object;
}

/** 通道工厂。 */
export interface ElectronChannelFactory {
  create(): ElectronStreamChannel;
}

/** 接收端口的窗口。 */
export interface ElectronStreamTarget {
  readonly id: number;
  isAlive(): boolean;
  send(ports: { readonly frames: object; readonly input: object }): void;
  onDestroyed(listener: () => void): void;
}

/** 用 MessageChannelMain 创建通道。 */
export function createElectronChannelFactory(): ElectronChannelFactory {
  return {
    create(): ElectronStreamChannel {
      const channel = new MessageChannelMain();
      return { first: channel.port1, second: channel.port2 };
    }
  };
}

/**
 * 把窗口当作端口的接收方。
 * @param contents 窗口的页面。
 * @param channel 主进程向页面发送端口用的 IPC 通道名。
 * @param tag 随端口发给页面的标记，预加载脚本据此确认消息的用途。
 * @returns 接收方。
 */
export function createWebContentsStreamTarget(contents: WebContents, channel: string, tag: string): ElectronStreamTarget {
  const id = contents.id;
  return {
    id,
    isAlive: () => !contents.isDestroyed(),
    send: (ports) => {
      contents.postMessage(channel, tag, [ports.frames as MessagePortMain, ports.input as MessagePortMain]);
    },
    onDestroyed: (listener) => {
      contents.once('destroyed', listener);
    }
  };
}
