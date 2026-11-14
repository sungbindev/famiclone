/*
 * ------------------------------------------------------------------------
 * 名称：pointer-capture.test.ts
 * 说明：鼠标采集的测试：指针位置换算成标准帧坐标（比例、缩放方式、画面之外）、位移、按键位、滚轮、离开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { MouseInputMessage } from '../../contracts/stream-ports/stream-messages';
import { MouseCapture, toFramePoint, type PointerBounds, type PointerView } from './pointer-capture';

/** 画面区 512x480 CSS 像素，画布与它一一对应。 */
const BOUNDS: PointerBounds = { left: 0, top: 0, width: 512, height: 480 };
const VIEW: PointerView = { canvasWidth: 512, canvasHeight: 480, aspect: '8:7', scaling: 'fit', cropLines: 0 };

describe('toFramePoint', () => {
  it('8:7 的画面在 512x480 里宽 512、高 420、上下各留 30：中心是帧的中心', () => {
    expect(toFramePoint(256, 240, BOUNDS, VIEW)).toEqual({ x: 128, y: 120 });
  });

  it('画面的四个角是 0 与 256、0 与 240', () => {
    expect(toFramePoint(0, 30, BOUNDS, VIEW)).toEqual({ x: 0, y: 0 });
    expect(toFramePoint(512, 450, BOUNDS, VIEW)).toEqual({ x: 256, y: 240 });
  });

  it('指针在画面之外（黑边或画面区外）时坐标超出范围', () => {
    expect(toFramePoint(256, 0, BOUNDS, VIEW)).toEqual({ x: 128, y: -17.14 });
    expect(toFramePoint(-10, 240, BOUNDS, VIEW)).toEqual({ x: -5, y: 120 });
  });

  it('画面区有偏移与缩放（画布的设备像素是 CSS 像素的两倍）时照样换算', () => {
    const bounds: PointerBounds = { left: 100, top: 50, width: 256, height: 240 };
    const view: PointerView = { canvasWidth: 512, canvasHeight: 480, aspect: '8:7', scaling: 'fit', cropLines: 0 };
    expect(toFramePoint(228, 170, bounds, view)).toEqual({ x: 128, y: 120 });
  });

  it('整数倍缩放按实际画面矩形换算', () => {
    // 画布 600x500：8:7 的整数倍取 2 倍，画面 480 高、585 宽，左右各留 7、上下各留 10。
    const view: PointerView = { canvasWidth: 600, canvasHeight: 500, aspect: '8:7', scaling: 'integer', cropLines: 0 };
    const point = toFramePoint(300, 250, { left: 0, top: 0, width: 600, height: 500 }, view);
    expect(point).toEqual({ x: 128.22, y: 120 });
  });

  it('4:3 的画面比例按 4:3 的矩形换算', () => {
    const view: PointerView = { canvasWidth: 640, canvasHeight: 480, aspect: '4:3', scaling: 'fit', cropLines: 0 };
    expect(toFramePoint(320, 240, { left: 0, top: 0, width: 640, height: 480 }, view)).toEqual({ x: 128, y: 120 });
    expect(toFramePoint(0, 0, { left: 0, top: 0, width: 640, height: 480 }, view)).toEqual({ x: 0, y: 0 });
  });

  it('上下各裁掉 8 行时，画面显示第 8 到 232 行，坐标仍是整帧上的行号', () => {
    // 4:3 的 640x480 画布：画面铺满，显示 224 行；画面最上边是第 8 行，最下边是第 232 行，中心不变。
    const bounds: PointerBounds = { left: 0, top: 0, width: 640, height: 480 };
    const view: PointerView = { canvasWidth: 640, canvasHeight: 480, aspect: '4:3', scaling: 'fit', cropLines: 8 };
    expect(toFramePoint(0, 0, bounds, view)).toEqual({ x: 0, y: 8 });
    expect(toFramePoint(640, 480, bounds, view)).toEqual({ x: 256, y: 232 });
    expect(toFramePoint(320, 240, bounds, view)).toEqual({ x: 128, y: 120 });
  });

  it('裁剪后 8:7 的显示宽高比按 256x224 计算', () => {
    // 256x224 按 8:7 的显示宽高比是 256*8/7/224 = 1.306，画布 512x480 的宽高比是 1.067：宽度铺满 512，高度 392，上下各留 44。
    const bounds: PointerBounds = { left: 0, top: 0, width: 512, height: 480 };
    const view: PointerView = { canvasWidth: 512, canvasHeight: 480, aspect: '8:7', scaling: 'fit', cropLines: 8 };
    expect(toFramePoint(0, 44, bounds, view)).toEqual({ x: 0, y: 8 });
    expect(toFramePoint(512, 436, bounds, view)).toEqual({ x: 256, y: 232 });
  });

  it('裁剪时指针在画面之外也按同一换算超出范围', () => {
    const bounds: PointerBounds = { left: 0, top: 0, width: 640, height: 480 };
    const view: PointerView = { canvasWidth: 640, canvasHeight: 480, aspect: '4:3', scaling: 'fit', cropLines: 8 };
    expect(toFramePoint(320, -48, bounds, view)).toEqual({ x: 128, y: -14.4 });
  });

  it('画面区或画布的尺寸无效时没有坐标', () => {
    expect(toFramePoint(1, 1, { left: 0, top: 0, width: 0, height: 480 }, VIEW)).toBeUndefined();
    expect(toFramePoint(1, 1, BOUNDS, { ...VIEW, canvasWidth: 0 })).toBeUndefined();
  });
});

function setupMouse() {
  const sent: MouseInputMessage[] = [];
  return { sent, capture: new MouseCapture((message) => sent.push(message)) };
}

describe('MouseCapture', () => {
  it('第一次事件没有位移，之后带着相对上一次的位移', () => {
    const { sent, capture } = setupMouse();
    capture.report('move', { x: 10, y: 20 }, 0);
    capture.report('move', { x: 12.5, y: 18 }, 0);
    expect(sent.map((message) => [message.dx, message.dy])).toEqual([
      [0, 0],
      [2.5, -2]
    ]);
  });

  it('按下、松开与滚轮带着按键位与滚动量，按键位只取低三位', () => {
    const { sent, capture } = setupMouse();
    capture.report('down', { x: 1, y: 2 }, 3);
    capture.report('wheel', { x: 1, y: 2 }, 3, -1.5);
    capture.report('up', { x: 1, y: 2 }, 0xff);
    expect(sent).toEqual([
      { type: 'mouse', kind: 'down', x: 1, y: 2, dx: 0, dy: 0, buttons: 3, wheel: 0 },
      { type: 'mouse', kind: 'wheel', x: 1, y: 2, dx: 0, dy: 0, buttons: 3, wheel: -1.5 },
      { type: 'mouse', kind: 'up', x: 1, y: 2, dx: 0, dy: 0, buttons: 7, wheel: 0 }
    ]);
  });

  it('离开时按最后的位置上报，之后的第一次移动不带位移', () => {
    const { sent, capture } = setupMouse();
    capture.report('move', { x: 5, y: 6 }, 0);
    capture.leave(1);
    capture.report('move', { x: 100, y: 100 }, 0);
    expect(sent[1]).toEqual({ type: 'mouse', kind: 'leave', x: 5, y: 6, dx: 0, dy: 0, buttons: 1, wheel: 0 });
    expect(sent[2]).toMatchObject({ kind: 'move', dx: 0, dy: 0 });
  });

  it('没有移动过就离开时位置为 0', () => {
    const { sent, capture } = setupMouse();
    capture.leave(0);
    expect(sent).toEqual([{ type: 'mouse', kind: 'leave', x: 0, y: 0, dx: 0, dy: 0, buttons: 0, wheel: 0 }]);
  });
});
