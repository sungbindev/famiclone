/*
 * ------------------------------------------------------------------------
 * 名称：transport.ts
 * 说明：传输层的抽象：一条已建立的连接（文本帧发送、报文接收、关闭通知）与建立连接的工厂。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：默认实现基于全局的 WebSocket（Node 22、浏览器、Electron 页面都有）；测试与特殊环境可以自己实现。
 * ------------------------------------------------------------------------
 */

/** 连接关闭的信息。 */
export interface CloseInfo {
  /** 关闭码；没有收到关闭帧时是 1006。 */
  readonly code: number;
  readonly reason: string;
}

/** 一条已建立的连接。 */
export interface Transport {
  /** 发送一个文本帧。 */
  send(text: string): void;
  /** 关闭连接。 */
  close(code?: number, reason?: string): void;
  /** 注册报文处理函数：文本帧给字符串，二进制帧给字节；注册之前收到的报文会补发。 */
  onMessage(handler: (message: string | Uint8Array) => void): void;
  /** 注册关闭处理函数；连接已关闭时立即回调。 */
  onClose(handler: (info: CloseInfo) => void): void;
}

/** 建立连接：成功时返回已打开的连接，失败时抛出 EmberConnectionError。 */
export type TransportFactory = (url: string, subprotocols: readonly string[]) => Promise<Transport>;
