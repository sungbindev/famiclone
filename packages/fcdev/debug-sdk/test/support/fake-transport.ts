/*
 * ------------------------------------------------------------------------
 * 名称：fake-transport.ts
 * 说明：测试用的假传输层：记录发出的请求，由测试代码或自动应答函数回报文，并可模拟服务端关闭连接。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import type { CloseInfo, Transport } from '../../src/contracts/transport.js';

/** 发出的请求。 */
export interface SentRequest {
  readonly jsonrpc: '2.0';
  readonly id: number;
  readonly method: string;
  readonly params?: Record<string, unknown>;
}

/** 自动应答：返回要回的 result（对象）、{ error } 或 undefined（不回）。 */
export type Responder = (request: SentRequest) => { result: unknown } | { error: { code: number; message: string; data?: { code: string } } } | undefined;

/** 假传输层。 */
export class FakeTransport implements Transport {
  readonly sent: SentRequest[] = [];
  closeArguments?: { code?: number; reason?: string };
  private messageHandler?: (message: string | Uint8Array) => void;
  private closeHandler?: (info: CloseInfo) => void;

  /** @param responder 每收到一个请求就调用，用来自动回响应。 */
  constructor(private readonly responder?: Responder) {}

  send(text: string): void {
    const request = JSON.parse(text) as SentRequest;
    this.sent.push(request);
    const reply = this.responder?.(request);
    if (reply !== undefined) queueMicrotask(() => this.receive(JSON.stringify({ jsonrpc: '2.0', id: request.id, ...reply })));
  }

  close(code?: number, reason?: string): void {
    this.closeArguments = { code, reason };
    this.closeFromServer(code ?? 1000, reason ?? '');
  }

  onMessage(handler: (message: string | Uint8Array) => void): void {
    this.messageHandler = handler;
  }

  onClose(handler: (info: CloseInfo) => void): void {
    this.closeHandler = handler;
  }

  /** 模拟服务端发来一个报文。 */
  receive(message: string | Uint8Array): void {
    this.messageHandler?.(message);
  }

  /** 模拟服务端发来一条通知。 */
  notify(method: string, params: unknown): void {
    this.receive(JSON.stringify({ jsonrpc: '2.0', method, params }));
  }

  /** 模拟连接被关闭。 */
  closeFromServer(code: number, reason = ''): void {
    this.closeHandler?.({ code, reason });
  }
}

/** 构造二进制帧：4 字节大端长度、JSON 报文、数据。 */
export function binaryFrame(message: object, data: Uint8Array): Uint8Array {
  const json = new TextEncoder().encode(JSON.stringify(message));
  const frame = new Uint8Array(4 + json.byteLength + data.byteLength);
  new DataView(frame.buffer).setUint32(0, json.byteLength, false);
  frame.set(json, 4);
  frame.set(data, 4 + json.byteLength);
  return frame;
}

/** 握手的默认应答。 */
export const HELLO_RESULT = { protocol: 1, coreVersion: '0.1.0', appVersion: '0.1.0', session: 1 };
