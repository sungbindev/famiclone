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

/** 键盘：一个物理键按下或松开；界面不映射，由核心按机器与手柄绑定处理。 */
export interface KeyInputMessage {
  readonly type: 'key';
  /** 物理键位，取 KeyboardEvent.code，如 KeyW、Space、Numpad1。 */
  readonly code: string;
  /** true 为按下，false 为松开。 */
  readonly down: boolean;
}

/** 鼠标事件的种类。 */
export type MouseKind = 'move' | 'down' | 'up' | 'wheel' | 'leave';

/** 鼠标：位置、按键与滚轮；每次发送事件之后的当前状态。 */
export interface MouseInputMessage {
  readonly type: 'mouse';
  readonly kind: MouseKind;
  /** 画面坐标，单位是标准帧（256x240）的像素；指针在画面之外时可以超出这个范围。 */
  readonly x: number;
  readonly y: number;
  /** 相对上一次事件的位移，单位同上。 */
  readonly dx: number;
  readonly dy: number;
  /** 当前按住的按键位：1 左键、2 右键、4 中键。 */
  readonly buttons: number;
  /** 滚轮的滚动量，向下为正；只有 wheel 事件不为 0。 */
  readonly wheel: number;
}

/** 游戏手柄：一个手柄当前的全部状态，顺序同浏览器 Gamepad API 的标准映射；核心按手柄绑定把它变成手柄口的按键。 */
export interface GamepadInputMessage {
  readonly type: 'gamepad';
  /** 手柄编号，同 Gamepad.index。 */
  readonly index: number;
  /** 设备标识，同 Gamepad.id（含设备名）；手柄绑定按它认出是哪个手柄。 */
  readonly id: string;
  /** 是否连着；拔掉时为 false，按键与轴为空数组。 */
  readonly connected: boolean;
  /** 各按键是否按下。 */
  readonly buttons: readonly boolean[];
  /** 各轴的值，范围 -1 到 1。 */
  readonly axes: readonly number[];
}

/** 渲染进程发给核心的输入：原始事件，没有“这是哪个手柄键”的含义。 */
export type InputMessage = KeyInputMessage | MouseInputMessage | GamepadInputMessage;

/** 预加载脚本把两个端口交给页面时，窗口消息的类型：端口数组为 [画面端口, 输入端口]。 */
export const STREAM_PORTS_WINDOW_MESSAGE = 'ember.stream-ports' as const;

/** 预加载脚本交给页面的窗口消息；两个端口在事件的 ports 里。 */
export interface StreamPortsWindowMessage {
  readonly type: typeof STREAM_PORTS_WINDOW_MESSAGE;
}
