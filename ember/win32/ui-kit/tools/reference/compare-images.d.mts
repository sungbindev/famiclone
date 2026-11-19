/*
 * ------------------------------------------------------------------------
 * 名称：compare-images.d.mts
 * 说明：compare-images.mjs 的类型声明，供 TypeScript 测试引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

/** 位置与大小（逻辑像素）。 */
export interface LayoutRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

/** 设备像素矩形，右、下为开区间。 */
export interface PixelBox {
  left: number;
  top: number;
  right: number;
  bottom: number;
}

/** RGBA 图像。 */
export interface Image {
  width: number;
  height: number;
  data: Uint8Array;
}

/** 对照两组控件位置。 */
export function compareLayout(expected: LayoutRect[], actual: LayoutRect[]): { maxDelta: number; deltas: ({ index: number } & LayoutRect)[] };
/** 把图裁成指定大小。 */
export function cropTo(image: Image, width: number, height: number): Image;
/** 逐像素对照。 */
export function comparePixels(expected: Image, actual: Image, masks: PixelBox[], tolerance: number): { total: number; mismatched: number; maxDiff: number; worst: { x: number; y: number } | null };
/** 提取与背景不同的像素的外框。 */
export function contentBox(image: Image, area: PixelBox, background: number[], threshold: number): PixelBox | null;
