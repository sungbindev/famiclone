/*
 * ------------------------------------------------------------------------
 * 名称：host-streams.ts
 * 说明：核心宿主的画面与输入端口：保存主进程交来的两个端口，把核心的帧发到画面端口，从输入端口接收键盘、鼠标、游戏手柄的原始事件并校验后交给回调。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只做转接与校验。帧消息的像素是核心复用的缓冲，端口的 postMessage 在调用时就复制数据，所以回调返回后缓冲可以被核心复用；
 *       不能把缓冲放进转移列表（转移会使核心的缓冲失效）。端口已关闭或页面消失时发送失败，帧丢弃，不影响核心。
 *       背压：已发出、渲染进程还没确认的帧最多 MAX_FRAMES_IN_FLIGHT 帧，超过时丢弃新帧并计数，渲染进程卡住时帧不会在端口队列里堆积。
 * ------------------------------------------------------------------------
 */

import { FRAME_PIXEL_FORMAT, MAX_FRAMES_IN_FLIGHT, type FrameAckMessage, type FrameMessage, type InputMessage, type MouseKind } from '../contracts/stream-ports/stream-messages';

/** 键位名的最大长度：KeyboardEvent.code 的名字都很短。 */
const MAX_KEY_CODE_LENGTH = 32;
/** 鼠标坐标、位移与滚轮量的绝对值上限。 */
const MAX_MOUSE_VALUE = 1_000_000;
/** 鼠标按键位的最大值：左、右、中键三位。 */
const MAX_MOUSE_BUTTONS = 7;
/** 游戏手柄的最大编号、按键数与轴数。 */
const MAX_GAMEPAD_INDEX = 15;
const MAX_GAMEPAD_BUTTONS = 32;
const MAX_GAMEPAD_AXES = 16;
/** 手柄设备标识的最大长度。 */
const MAX_GAMEPAD_ID_LENGTH = 256;

/** 鼠标事件的全部种类。 */
const MOUSE_KINDS: ReadonlySet<string> = new Set<MouseKind>(['move', 'down', 'up', 'wheel', 'leave']);

/** 端口的最小接口，Electron 的 MessagePortMain 满足它。 */
export interface HostPort {
  postMessage(message: unknown): void;
  on(event: 'message', listener: (event: { readonly data: unknown }) => void): unknown;
  on(event: 'close', listener: () => void): unknown;
  start(): void;
  close(): void;
}

/** 在 limit 之内的有限数。 */
function isBoundedNumber(value: unknown, limit: number): value is number {
  return typeof value === 'number' && Number.isFinite(value) && Math.abs(value) <= limit;
}

/** 0 到 maximum 的整数。 */
function isIntegerUpTo(value: unknown, maximum: number): value is number {
  return typeof value === 'number' && Number.isInteger(value) && value >= 0 && value <= maximum;
}

/**
 * 校验渲染进程发来的输入消息：输入来自不可信的页面，格式不对就丢弃，多余的字段去掉。
 * @param value 收到的数据。
 * @returns 合格的消息；不合格时为 undefined。
 */
export function parseInputMessage(value: unknown): InputMessage | undefined {
  if (typeof value !== 'object' || value === null) return undefined;
  const fields = value as Record<string, unknown>;
  switch (fields.type) {
    case 'key': {
      const { code, down } = fields;
      if (typeof code !== 'string' || code.length === 0 || code.length > MAX_KEY_CODE_LENGTH || !/^[A-Za-z0-9]+$/.test(code)) return undefined;
      if (typeof down !== 'boolean') return undefined;
      return { type: 'key', code, down };
    }
    case 'mouse': {
      const { kind, x, y, dx, dy, buttons, wheel } = fields;
      if (typeof kind !== 'string' || !MOUSE_KINDS.has(kind)) return undefined;
      if (!isBoundedNumber(x, MAX_MOUSE_VALUE) || !isBoundedNumber(y, MAX_MOUSE_VALUE) || !isBoundedNumber(dx, MAX_MOUSE_VALUE) || !isBoundedNumber(dy, MAX_MOUSE_VALUE)) return undefined;
      if (!isBoundedNumber(wheel, MAX_MOUSE_VALUE) || !isIntegerUpTo(buttons, MAX_MOUSE_BUTTONS)) return undefined;
      return { type: 'mouse', kind: kind as MouseKind, x, y, dx, dy, buttons, wheel };
    }
    case 'gamepad': {
      const { index, id, connected, buttons, axes } = fields;
      if (!isIntegerUpTo(index, MAX_GAMEPAD_INDEX) || typeof connected !== 'boolean') return undefined;
      if (typeof id !== 'string' || id.length > MAX_GAMEPAD_ID_LENGTH) return undefined;
      if (!Array.isArray(buttons) || buttons.length > MAX_GAMEPAD_BUTTONS || !buttons.every((item) => typeof item === 'boolean')) return undefined;
      if (!Array.isArray(axes) || axes.length > MAX_GAMEPAD_AXES || !axes.every((item) => isBoundedNumber(item, 1))) return undefined;
      return { type: 'gamepad', index, id, connected, buttons: buttons as boolean[], axes: axes as number[] };
    }
    default:
      return undefined;
  }
}

/**
 * 校验渲染进程在画面端口上回的确认：同样来自不可信的页面，格式不对就丢弃。
 * @param value 收到的数据。
 * @returns 合格的确认；不合格时为 undefined。
 */
export function parseAckMessage(value: unknown): FrameAckMessage | undefined {
  if (typeof value !== 'object' || value === null) return undefined;
  const { type, frame } = value as Record<string, unknown>;
  if (type !== 'ack') return undefined;
  if (typeof frame !== 'number' || !Number.isInteger(frame) || frame < 0) return undefined;
  return { type, frame };
}

/** 画面与输入端口。 */
export class HostStreams {
  private frames: HostPort | undefined;
  private input: HostPort | undefined;
  /** 已发出、还没确认的帧号，按发出顺序排列。 */
  private inFlight: number[] = [];
  /** 当前这组端口上因背压丢弃的帧数。 */
  private droppedByBackpressure = 0;

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
    frames.on('message', (event) => {
      if (this.frames === frames) this.acknowledge(parseAckMessage(event.data));
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
    this.inFlight = [];
    this.droppedByBackpressure = 0;
    for (const port of ports) {
      try {
        port?.close();
      } catch {
        // 端口已经关闭。
      }
    }
  }

  /** 在途（已发出、未确认）的帧数。 */
  get framesInFlight(): number {
    return this.inFlight.length;
  }

  /** 当前这组端口上因背压丢弃的帧数。 */
  get backpressureDropped(): number {
    return this.droppedByBackpressure;
  }

  /**
   * 发送一帧；没有端口或发送失败时丢弃，在途帧已满时丢弃并计数。
   * @param pixels 像素缓冲，只在本次调用期间有效。
   * @param publishedAt 发布时间（Date.now()，毫秒），随帧发给渲染进程用于测量延迟。
   */
  sendFrame(pixels: ArrayBuffer, width: number, height: number, frame: number, publishedAt: number): void {
    if (this.frames === undefined) return;
    if (this.inFlight.length >= MAX_FRAMES_IN_FLIGHT) {
      this.droppedByBackpressure += 1;
      return;
    }
    const message: FrameMessage = { type: 'frame', width, height, format: FRAME_PIXEL_FORMAT, frame, publishedAt, hostDropped: this.droppedByBackpressure, pixels };
    try {
      this.frames.postMessage(message);
      this.inFlight.push(frame);
    } catch {
      // 页面已经消失，这一帧丢弃。
    }
  }

  /** 收到确认：释放最早的同号在途帧；没有这一帧（重复或伪造的确认）时忽略。 */
  private acknowledge(ack: FrameAckMessage | undefined): void {
    if (ack === undefined) return;
    const index = this.inFlight.indexOf(ack.frame);
    if (index >= 0) this.inFlight.splice(index, 1);
  }
}
