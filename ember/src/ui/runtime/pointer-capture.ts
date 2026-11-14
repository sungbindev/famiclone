/*
 * ------------------------------------------------------------------------
 * 名称：pointer-capture.ts
 * 说明：鼠标采集：把指针在画面区里的位置换算成标准帧（256x240）的坐标，并把移动、按下、松开、滚轮、离开转成鼠标事件消息。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯逻辑，不监听任何事件，由画面组件把指针事件交进来。坐标按画面在画布里的实际矩形换算（见 screen-layout.ts），
 *       所以不受画面比例、缩放方式与窗口大小的影响；指针在画面之外时坐标可以超出 0 到 256、0 到 240。界面不解释这些事件，由卡带的 mapper 脚本决定怎么用。
 * ------------------------------------------------------------------------
 */

import type { MouseInputMessage, MouseKind } from '../../contracts/stream-ports/stream-messages';
import { computeScreenRect, NOMINAL_FRAME, type ScreenAspect, type ScreenScaling } from './screen-layout';

/** 画面区元素的位置与大小（CSS 像素，同 getBoundingClientRect）。 */
export interface PointerBounds {
  readonly left: number;
  readonly top: number;
  readonly width: number;
  readonly height: number;
}

/** 画面当前的显示方式：画布的设备像素尺寸、显示选项与上下各裁掉的行数。 */
export interface PointerView {
  readonly canvasWidth: number;
  readonly canvasHeight: number;
  readonly aspect: ScreenAspect;
  readonly scaling: ScreenScaling;
  /** 帧上下各裁掉的行数；显示的是中间的行，坐标仍然是整帧上的行号。 */
  readonly cropLines: number;
}

/** 标准帧上的一个点。 */
export interface FramePoint {
  readonly x: number;
  readonly y: number;
}

/** 坐标保留的小数位：两位已经比一个像素的百分之一更细。 */
const COORDINATE_SCALE = 100;

function round(value: number): number {
  return Math.round(value * COORDINATE_SCALE) / COORDINATE_SCALE;
}

/**
 * 把指针位置换算成标准帧的坐标。
 * @param clientX 指针的横坐标（CSS 像素，相对视口）。
 * @param clientY 指针的纵坐标。
 * @param bounds 画面区元素的位置与大小。
 * @param view 画布尺寸与显示选项。
 * @returns 标准帧坐标；画面区或画面的尺寸无效（还没有画布）时为 undefined。
 */
export function toFramePoint(clientX: number, clientY: number, bounds: PointerBounds, view: PointerView): FramePoint | undefined {
  if (!(bounds.width > 0 && bounds.height > 0)) return undefined;
  const visibleHeight = NOMINAL_FRAME.height - view.cropLines * 2;
  const rect = computeScreenRect(view.canvasWidth, view.canvasHeight, NOMINAL_FRAME.width, visibleHeight, view.aspect, view.scaling);
  if (rect.width === 0 || rect.height === 0) return undefined;
  const deviceX = ((clientX - bounds.left) * view.canvasWidth) / bounds.width;
  const deviceY = ((clientY - bounds.top) * view.canvasHeight) / bounds.height;
  // 矩形的原点在画布左下角，换算成从上往下数；裁掉的行在上面，行号要加上它。
  const rectTop = view.canvasHeight - (rect.y + rect.height);
  return { x: round(((deviceX - rect.x) / rect.width) * NOMINAL_FRAME.width), y: round(view.cropLines + ((deviceY - rectTop) / rect.height) * visibleHeight) };
}

/** 鼠标采集：记住上一次的位置，把每次事件转成带位移的消息。 */
export class MouseCapture {
  private last: FramePoint | undefined;

  /** @param send 每个鼠标事件调用一次。 */
  constructor(private readonly send: (message: MouseInputMessage) => void) {}

  /**
   * 上报一次鼠标事件。
   * @param kind 事件种类。
   * @param point 指针在标准帧上的位置。
   * @param buttons 事件之后按住的按键位：1 左键、2 右键、4 中键。
   * @param wheel 滚轮的滚动量，向下为正，只有 wheel 事件不为 0。
   */
  report(kind: MouseKind, point: FramePoint, buttons: number, wheel = 0): void {
    const previous = this.last ?? point;
    this.last = point;
    this.send({ type: 'mouse', kind, x: point.x, y: point.y, dx: round(point.x - previous.x), dy: round(point.y - previous.y), buttons: buttons & 7, wheel });
  }

  /** 指针离开画面区：按最后的位置上报一次 leave，之后的第一次移动不带位移。 */
  leave(buttons: number): void {
    const point = this.last ?? { x: 0, y: 0 };
    this.send({ type: 'mouse', kind: 'leave', x: point.x, y: point.y, dx: 0, dy: 0, buttons: buttons & 7, wheel: 0 });
    this.last = undefined;
  }
}
