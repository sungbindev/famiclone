/*
 * ------------------------------------------------------------------------
 * 名称：stream-ports.ts
 * 说明：画面与输入消息端口的抽象：端口通道工厂、接收端口的窗口、核心进程的端口入口，以及预加载脚本暴露给页面的方法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：端口的具体类型（Electron 的 MessagePortMain）只在 platform 层出现；其余层把它当作不透明的句柄传递。
 * ------------------------------------------------------------------------
 */

/** 消息端口句柄：不透明，只能原样交给端口的另一方。 */
export type StreamPort = object;

/** 一条通道的两端。 */
export interface StreamChannel {
  readonly first: StreamPort;
  readonly second: StreamPort;
}

/** 画面通道与输入通道的一端：核心拿到的是各自的 first，窗口拿到的是 second。 */
export interface StreamPortPair {
  /** 画面通道：核心发，渲染进程收。 */
  readonly frames: StreamPort;
  /** 输入通道：渲染进程发，核心收。 */
  readonly input: StreamPort;
}

/** 通道工厂。 */
export interface StreamChannelFactory {
  /** 创建一条新通道。 */
  create(): StreamChannel;
}

/** 接收端口的窗口。 */
export interface StreamTarget {
  /** 窗口的标识，同一个窗口重复请求端口时标识相同。 */
  readonly id: number;
  /** 窗口是否还在。 */
  isAlive(): boolean;
  /** 把端口交给窗口的页面；窗口已销毁时抛出异常。 */
  send(ports: StreamPortPair): void;
  /** 窗口销毁时通知。 */
  onDestroyed(listener: () => void): void;
}

/** 核心进程的端口入口。 */
export interface CoreStreamSink {
  /**
   * 把端口交给核心进程；核心进程不在运行（正在启动失败、重启中或停止）时返回 false。
   * 已有的端口被核心关闭并替换。
   */
  attachStreams(ports: StreamPortPair): boolean;
  /** 核心进程每次启动（含意外退出后重启）时通知；返回取消订阅的函数。 */
  onCoreStarted(listener: () => void): () => void;
}

/** 窗口请求端口时的入口：记住窗口并给它建立端口。 */
export interface StreamConnector {
  /** 窗口请求端口；同一个窗口重复请求时给它新的端口，旧端口作废。 */
  connect(target: StreamTarget): void;
}

/** 预加载脚本暴露给页面的端口方法（window.ember.streams）。 */
export interface StreamPortsBridge {
  /**
   * 请求画面与输入端口。端口以窗口消息交给页面（见 stream-messages.ts 的 StreamPortsWindowMessage），
   * 页面先监听 message 事件并检查 event.source 为自己的窗口，再调用本方法；
   * 页面重载或核心进程重启后会收到新的端口，旧端口作废。
   */
  requestPorts(): void;
}

/** 端口相关的 IPC 通道名。 */
export const STREAM_CHANNELS = {
  /** 页面请求端口。 */
  connect: 'streams:connect',
  /** 主进程把端口交给页面。 */
  ports: 'streams:ports'
} as const;
