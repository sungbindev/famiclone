/*
 * ------------------------------------------------------------------------
 * 名称：pad-layout.ts
 * 说明：手柄图的版面：画布尺寸，手柄外形与十字键中心的位置，以及 10 个键各自在图上的位置与大小。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：坐标与尺寸用设计单位（画布 560x220），界面按比例换成百分比，窗口多宽图就多大；不依赖界面，便于测试键不出界、不重叠。
 *       十字键的四个方向是四个相邻的矩形，中间那块只是装饰。
 * ------------------------------------------------------------------------
 */

import type { PadButtonName } from './input-model';

/** 画布尺寸（设计单位）。 */
export const PAD_CANVAS = { width: 560, height: 220 } as const;

/** 手柄外形：圆角矩形。 */
export const PAD_BODY = { x: 4, y: 4, width: 552, height: 212, radius: 36 } as const;

/** 十字键中间的装饰块。 */
export const PAD_CROSS_CENTER = { x: 80, y: 88, width: 60, height: 44 } as const;

/** 一个键在图上的位置：中心点与大小；圆形键宽高相等。 */
export interface PadKeyLayout {
  readonly id: PadButtonName;
  readonly shape: 'rect' | 'round';
  readonly cx: number;
  readonly cy: number;
  readonly width: number;
  readonly height: number;
}

/** 方向键、选择与开始键是矩形，A、B 与连发键是圆形。 */
export const PAD_LAYOUT: readonly PadKeyLayout[] = [
  { id: 'up', shape: 'rect', cx: 110, cy: 66, width: 60, height: 44 },
  { id: 'down', shape: 'rect', cx: 110, cy: 154, width: 60, height: 44 },
  { id: 'left', shape: 'rect', cx: 50, cy: 110, width: 60, height: 44 },
  { id: 'right', shape: 'rect', cx: 170, cy: 110, width: 60, height: 44 },
  { id: 'select', shape: 'rect', cx: 232, cy: 168, width: 84, height: 34 },
  { id: 'start', shape: 'rect', cx: 328, cy: 168, width: 84, height: 34 },
  { id: 'turboB', shape: 'round', cx: 420, cy: 60, width: 64, height: 64 },
  { id: 'turboA', shape: 'round', cx: 500, cy: 60, width: 64, height: 64 },
  { id: 'b', shape: 'round', cx: 420, cy: 140, width: 64, height: 64 },
  { id: 'a', shape: 'round', cx: 500, cy: 140, width: 64, height: 64 }
];

/** 画布上的百分比位置与大小（相对画布宽、高）。 */
export interface PadKeyPercent {
  readonly left: number;
  readonly top: number;
  readonly width: number;
  /** 圆形键的高度由宽度和宽高比决定，没有这一项。 */
  readonly height: number | undefined;
}

/**
 * 把键的位置换成画布上的百分比。
 * @param key 键的版面。
 */
export function percentOf(key: PadKeyLayout): PadKeyPercent {
  return {
    left: (key.cx / PAD_CANVAS.width) * 100,
    top: (key.cy / PAD_CANVAS.height) * 100,
    width: (key.width / PAD_CANVAS.width) * 100,
    height: key.shape === 'round' ? undefined : (key.height / PAD_CANVAS.height) * 100
  };
}
