/*
 * ------------------------------------------------------------------------
 * 名称：stream-messages.ts
 * 说明：画面与输入消息端口上的消息格式：核心到渲染进程的帧消息、渲染进程到核心的输入消息，以及预加载脚本把端口交给页面时用的窗口消息。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：帧与输入不走请求，各用一条单独的消息端口（见 docs/01-软件架构设计.md 第 7 节）；像素格式见 docs/02-硬件模块.md 第 6.1 节。
 *       画面端口是双向的：核心发帧，渲染进程回确认；在途（已发出、未确认）的帧不超过 MAX_FRAMES_IN_FLIGHT。
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
  /** 发布时间：核心宿主发出这一帧时的 Date.now()（毫秒，核心进程与渲染进程用同一台机器的时钟），用于测量延迟。 */
  readonly publishedAt: number;
  /** 核心宿主因在途帧过多（背压）而累计丢弃的帧数，换端口后从 0 重新计。 */
  readonly hostDropped: number;
  /** 像素：width × height × 4 字节，只属于这条消息，接收方可以保留。 */
  readonly pixels: ArrayBuffer;
}

/** 在途帧的上限：已发出但渲染进程还没确认的帧超过它时，核心宿主丢弃新帧。 */
export const MAX_FRAMES_IN_FLIGHT = 2;

/** 渲染进程在画面端口上回给核心的确认：这一帧已画出，或已被丢弃（不会再占用在途名额）。 */
export interface FrameAckMessage {
  readonly type: 'ack';
  /** 被确认的帧号。 */
  readonly frame: number;
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
