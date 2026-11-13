/*
 * ------------------------------------------------------------------------
 * 名称：host-streams.ts
 * 说明：核心宿主的画面与输入端口：保存主进程交来的两个端口，把核心的帧发到画面端口，从输入端口接收按键并校验后交给回调。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只做转接与校验。帧消息的像素是核心复用的缓冲，端口的 postMessage 在调用时就复制数据，所以回调返回后缓冲可以被核心复用；
 *       不能把缓冲放进转移列表（转移会使核心的缓冲失效）。端口已关闭或页面消失时发送失败，帧丢弃，不影响核心。
 * ------------------------------------------------------------------------
 */

import { FRAME_PIXEL_FORMAT, INPUT_PORT_COUNT, type FrameMessage, type InputMessage } from '../contracts/stream-ports/stream-messages';

/** 按键掩码的最大值。 */
const MAX_BUTTONS = 255;

/** 端口的最小接口，Electron 的 MessagePortMain 满足它。 */
export interface HostPort {
  postMessage(message: unknown): void;
  on(event: 'message', listener: (event: { readonly data: unknown }) => void): unknown;
  on(event: 'close', listener: () => void): unknown;
  start(): void;
  close(): void;
}

/**
 * 校验渲染进程发来的输入消息：输入来自不可信的页面，格式不对就丢弃。
 * @param value 收到的数据。
 * @returns 合格的消息；不合格时为 undefined。
 */
export function parseInputMessage(value: unknown): InputMessage | undefined {
  if (typeof value !== 'object' || value === null) return undefined;
  const { type, port, buttons } = value as Record<string, unknown>;
  if (type !== 'buttons') return undefined;
  if (typeof port !== 'number' || !Number.isInteger(port) || port < 0 || port >= INPUT_PORT_COUNT) return undefined;
  if (typeof buttons !== 'number' || !Number.isInteger(buttons) || buttons < 0 || buttons > MAX_BUTTONS) return undefined;
  return { type, port, buttons };
}

/** 画面与输入端口。 */
export class HostStreams {
  private frames: HostPort | undefined;
  private input: HostPort | undefined;

  /** @param onInput 收到合格的输入消息时调用。 */
  constructor(private readonly onInput: (message: InputMessage) => void) {}

  /**
   * 换用新的端口，已有的端口关闭。
   * @param ports 依次是画面端口与输入端口；个数不对时忽略。
   * @returns 是否采用了这组端口。
   */
  attach(ports: readonly HostPort[]): boolean {
    const [frames, input] = ports;
    if (ports.length !== 2 || frames === undefined || input === undefined) return false;
    this.detach();
    this.frames = frames;
    this.input = input;
    frames.on('close', () => {
      if (this.frames === frames) this.frames = undefined;
    });
    input.on('close', () => {
      if (this.input === input) this.input = undefined;
    });
    input.on('message', (event) => {
      const message = parseInputMessage(event.data);
      if (message !== undefined) this.onInput(message);
    });
    frames.start();
    input.start();
    return true;
  }

  /** 关闭并忘记端口。 */
  detach(): void {
    const ports = [this.frames, this.input];
    this.frames = undefined;
    this.input = undefined;
    for (const port of ports) {
      try {
        port?.close();
      } catch {
        // 端口已经关闭。
      }
    }
  }

  /**
   * 发送一帧；没有端口或发送失败时丢弃。
   * @param pixels 像素缓冲，只在本次调用期间有效。
   */
  sendFrame(pixels: ArrayBuffer, width: number, height: number, frame: number): void {
    if (this.frames === undefined) return;
    const message: FrameMessage = { type: 'frame', width, height, format: FRAME_PIXEL_FORMAT, frame, pixels };
    try {
      this.frames.postMessage(message);
    } catch {
      // 页面已经消失，这一帧丢弃。
    }
  }
}
