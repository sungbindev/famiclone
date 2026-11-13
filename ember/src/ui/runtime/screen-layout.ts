/*
 * ------------------------------------------------------------------------
 * 名称：screen-layout.ts
 * 说明：画面区域的版面计算：画面比例与缩放方式的取值，以及在画布里按比例居中放置画面的矩形。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯函数，单位是设备像素（画布的实际像素）。画面比例 4:3 表示整帧的显示宽高比为 4:3，8:7 表示像素宽高比为 8:7；
 *       整数倍按垂直方向取整数倍（画面高度是帧高度的整数倍），水平方向随比例而定。画面只影响显示，不影响模拟。
 * ------------------------------------------------------------------------
 */

/** 画面比例：4:3 为整帧的显示宽高比，8:7 为像素宽高比。 */
export type ScreenAspect = '4:3' | '8:7';

/** 缩放方式：适应窗口（尽量放大）或整数倍（放不下时取最大的整数倍）。 */
export type ScreenScaling = 'fit' | 'integer';

/** 默认的画面比例。 */
export const DEFAULT_SCREEN_ASPECT: ScreenAspect = '4:3';

/** 默认的缩放方式。 */
export const DEFAULT_SCREEN_SCALING: ScreenScaling = 'fit';

/** 像素宽高比 8:7 的宽与高。 */
const PIXEL_ASPECT_WIDTH = 8;
const PIXEL_ASPECT_HEIGHT = 7;

/** 画面在画布里的矩形：左下角为原点（WebGL 视口的约定），单位为设备像素。 */
export interface ScreenRect {
  readonly x: number;
  readonly y: number;
  readonly width: number;
  readonly height: number;
}

/**
 * 画面的显示宽高比（宽除以高）。
 * @param aspect 画面比例。
 * @param frameWidth 帧宽度（像素）。
 * @param frameHeight 帧高度（像素）。
 */
export function displayAspectRatio(aspect: ScreenAspect, frameWidth: number, frameHeight: number): number {
  if (aspect === '4:3') return 4 / 3;
  return (frameWidth * PIXEL_ASPECT_WIDTH) / PIXEL_ASPECT_HEIGHT / frameHeight;
}

/** 把宽高限制在画布内并居中。 */
function centered(canvasWidth: number, canvasHeight: number, width: number, height: number): ScreenRect {
  const fittedWidth = Math.max(0, Math.min(canvasWidth, Math.round(width)));
  const fittedHeight = Math.max(0, Math.min(canvasHeight, Math.round(height)));
  return { x: Math.floor((canvasWidth - fittedWidth) / 2), y: Math.floor((canvasHeight - fittedHeight) / 2), width: fittedWidth, height: fittedHeight };
}

/**
 * 计算画面在画布里的矩形：按显示比例居中，周围留给黑底。
 * @param canvasWidth 画布宽度（设备像素）。
 * @param canvasHeight 画布高度（设备像素）。
 * @param frameWidth 帧宽度（像素）。
 * @param frameHeight 帧高度（像素）。
 * @param aspect 画面比例。
 * @param scaling 缩放方式；整数倍连一倍都放不下时退回适应窗口。
 * @returns 矩形；画布或帧的尺寸无效时宽高为 0。
 */
export function computeScreenRect(canvasWidth: number, canvasHeight: number, frameWidth: number, frameHeight: number, aspect: ScreenAspect, scaling: ScreenScaling): ScreenRect {
  if (!(canvasWidth > 0 && canvasHeight > 0 && frameWidth > 0 && frameHeight > 0)) return { x: 0, y: 0, width: 0, height: 0 };
  const ratio = displayAspectRatio(aspect, frameWidth, frameHeight);
  if (scaling === 'integer') {
    const multiple = Math.floor(Math.min(canvasHeight / frameHeight, canvasWidth / (frameHeight * ratio)));
    if (multiple >= 1) return centered(canvasWidth, canvasHeight, frameHeight * multiple * ratio, frameHeight * multiple);
  }
  if (canvasWidth / canvasHeight >= ratio) return centered(canvasWidth, canvasHeight, canvasHeight * ratio, canvasHeight);
  return centered(canvasWidth, canvasHeight, canvasWidth, canvasWidth / ratio);
}
