/*
 * ------------------------------------------------------------------------
 * 名称：stream-messages.ts
 * 说明：画面与输入消息端口上的消息格式：核心到渲染进程的帧消息、渲染进程到核心的输入消息，以及预加载脚本把端口交给页面时用的窗口消息。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：帧与输入不走请求，各用一条单独的消息端口（见 docs/01-软件架构设计.md 第 7 节）；像素格式见 docs/02-硬件模块.md 第 6.1 节。
 * ------------------------------------------------------------------------
 */

/** 画面的像素格式：每像素 4 字节，内存顺序 R、G、B、A，可直接上传为 RGBA8 纹理。 */
export const FRAME_PIXEL_FORMAT = 'rgba8' as const;

/** 核心发给渲染进程的一帧画面。 */
export interface FrameMessage {
  readonly type: 'frame';
  /** 画面宽度（像素）。 */
  readonly width: number;
  /** 画面高度（像素）。 */
  readonly height: number;
  /** 像素格式。 */
  readonly format: typeof FRAME_PIXEL_FORMAT;
  /** 帧号：自上电以来完成的帧数，上电后从 1 重新开始。 */
  readonly frame: number;
  /** 像素：width × height × 4 字节，只属于这条消息，接收方可以保留。 */
  readonly pixels: ArrayBuffer;
}

/** 手柄口的个数，口号为 0 到 INPUT_PORT_COUNT - 1。 */
export const INPUT_PORT_COUNT = 2;

/** 手柄的按键：位 0 为 A，位 1 为 B，位 2 为选择，位 3 为开始，位 4 到 7 依次为上、下、左、右。 */
export interface ButtonsInputMessage {
  readonly type: 'buttons';
  /** 手柄口编号，0 或 1。 */
  readonly port: number;
  /** 按键掩码，0 到 255；每次发送整个手柄的当前状态。 */
  readonly buttons: number;
}

/** 渲染进程发给核心的输入。 */
export type InputMessage = ButtonsInputMessage;

/** 预加载脚本把两个端口交给页面时，窗口消息的类型：端口数组为 [画面端口, 输入端口]。 */
export const STREAM_PORTS_WINDOW_MESSAGE = 'ember.stream-ports' as const;

/** 预加载脚本交给页面的窗口消息；两个端口在事件的 ports 里。 */
export interface StreamPortsWindowMessage {
  readonly type: typeof STREAM_PORTS_WINDOW_MESSAGE;
}
