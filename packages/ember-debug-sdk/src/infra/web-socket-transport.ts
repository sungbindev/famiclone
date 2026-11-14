/*
 * ------------------------------------------------------------------------
 * 名称：web-socket-transport.ts
 * 说明：基于全局 WebSocket 的传输层：打开连接，缓存处理函数注册之前收到的报文，把关闭码带给上层。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：Node 22 以上、浏览器与 Electron 页面都有全局的 WebSocket；只用标准接口，不引用 Node 内置模块。
 * ------------------------------------------------------------------------
 */

import type { CloseInfo, Transport } from '../contracts/transport.js';
import { EmberConnectionError } from '../shared/errors.js';

/** 没有收到关闭帧就断开时的关闭码（RFC 6455 的 1006）。 */
const ABNORMAL_CLOSE_CODE = 1006;

/** 连接没有建立时的错误；有关闭码时带上。 */
function unavailable(url: string, closeCode?: number): EmberConnectionError {
  const reason = 'Ember 没有运行、没有开启调试服务、端口不对，或服务拒绝了连接';
  return new EmberConnectionError('unavailable', `无法连接调试服务 ${url}：${reason}${closeCode === undefined ? '' : `（关闭码 ${closeCode}）`}`, closeCode);
}

/** 一条打开的 WebSocket 连接。 */
class WebSocketTransport implements Transport {
  private messageHandler?: (message: string | Uint8Array) => void;
  private closeHandler?: (info: CloseInfo) => void;
  private readonly backlog: Array<string | Uint8Array> = [];
  private closeInfo?: CloseInfo;

  constructor(private readonly socket: WebSocket) {
    socket.addEventListener('message', (event: MessageEvent) => {
      const message = typeof event.data === 'string' ? event.data : new Uint8Array(event.data as ArrayBuffer);
      if (this.messageHandler === undefined) this.backlog.push(message);
      else this.messageHandler(message);
    });
    socket.addEventListener('close', (event: CloseEvent) => {
      this.closeInfo = { code: event.code, reason: event.reason };
      this.closeHandler?.(this.closeInfo);
    });
  }

  send(text: string): void {
    this.socket.send(text);
  }

  close(code?: number, reason?: string): void {
    this.socket.close(code, reason);
  }

  onMessage(handler: (message: string | Uint8Array) => void): void {
    this.messageHandler = handler;
    for (const message of this.backlog.splice(0)) handler(message);
  }

  onClose(handler: (info: CloseInfo) => void): void {
    this.closeHandler = handler;
    if (this.closeInfo !== undefined) handler(this.closeInfo);
  }
}

/**
 * 打开到调试服务的 WebSocket 连接。
 * @param url 如 ws://127.0.0.1:5688/。
 * @param subprotocols 请求的子协议。
 * @throws EmberConnectionError 当前环境没有 WebSocket，或连接没有建立。
 */
export function openWebSocket(url: string, subprotocols: readonly string[]): Promise<Transport> {
  const Socket = globalThis.WebSocket as typeof WebSocket | undefined;
  if (Socket === undefined) return Promise.reject(new EmberConnectionError('unavailable', '当前环境没有全局的 WebSocket，需要 Node 22 或更高版本'));
  return new Promise<Transport>((resolve, reject) => {
    let socket: WebSocket;
    try {
      socket = new Socket(url, [...subprotocols]);
    } catch {
      reject(new EmberConnectionError('unavailable', `调试服务的地址不合法：${url}`));
      return;
    }
    socket.binaryType = 'arraybuffer';
    const transport = new WebSocketTransport(socket);
    let opened = false;
    socket.addEventListener('open', () => {
      opened = true;
      resolve(transport);
    });
    socket.addEventListener('error', () => {
      if (!opened) reject(unavailable(url));
    });
    socket.addEventListener('close', (event: CloseEvent) => {
      if (!opened) reject(unavailable(url, event.code || ABNORMAL_CLOSE_CODE));
    });
  });
}
