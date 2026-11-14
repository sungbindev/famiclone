/*
 * ------------------------------------------------------------------------
 * 名称：panel-bounds.ts
 * 说明：窗口随面板伸缩的计算：窗口在面板全部关闭时的最小尺寸，展开面板后窗口的新边界与最小尺寸，以及从窗口边界扣掉面板得到要保存的基础边界。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯函数，不引用 Electron；边栏在窗口左侧，所以边栏展开时窗口向左伸（页面区在屏幕上的位置不动），右边栏向右、底部面板向下；
 *       屏幕工作区放不下时先把窗口平移进工作区，仍放不下就缩到工作区大小（此时页面区会变小）。
 * ------------------------------------------------------------------------
 */

/** 展开的面板占用的尺寸（像素），折叠的面板为 0；平台层不引用 shared，按结构与 shared/shell-state/panel-extents.ts 里的 PanelExtents 一致。 */
export interface PanelExtents {
  readonly left: number;
  readonly right: number;
  readonly bottom: number;
}

/** 全部面板都折叠。 */
export const NO_PANEL_EXTENTS: PanelExtents = { left: 0, right: 0, bottom: 0 };

/** 面板全部关闭时窗口的最小宽度：标题栏要放得下图标、名称、三个菜单、至少 48 像素可拖动的空白、三个折叠按钮和系统窗口按钮（实测这些加起来约 604）。 */
export const BASE_MIN_WIDTH = 640;
/** 面板全部关闭时窗口的最小高度：让画面框在最小窗口下四周间距一致，即页面区的内容宽高比等于画面框的 128:105；页面区内容宽 640-40(活动栏)-2x10(页面内边距)=580，对应画面高约 476，再加页面内边距 20、标题栏 36、标签栏 36、状态栏 26，共 594。 */
export const BASE_MIN_HEIGHT = 594;

/** 矩形。 */
export interface Rect {
  readonly x: number;
  readonly y: number;
  readonly width: number;
  readonly height: number;
}

/**
 * 展开面板后窗口的最小尺寸。
 * @param extents 展开的面板占用的尺寸。
 */
export function minimumSizeFor(extents: PanelExtents): { width: number; height: number } {
  return { width: BASE_MIN_WIDTH + extents.left + extents.right, height: BASE_MIN_HEIGHT + extents.bottom };
}

/** 把一段长度为 size 的区间放进 [areaStart, areaStart + areaSize]：放得下就平移进去，放不下就贴着起点。 */
function fit(start: number, size: number, areaStart: number, areaSize: number): number {
  if (size >= areaSize) return areaStart;
  return Math.min(Math.max(start, areaStart), areaStart + areaSize - size);
}

/**
 * 面板尺寸变化后窗口的新边界：窗口按差值向外伸缩（边栏向左、右边栏向右、底部面板向下），页面区的大小与位置保持不变。
 * @param bounds 窗口现在的边界。
 * @param from 变化前面板占用的尺寸。
 * @param to 变化后面板占用的尺寸。
 * @param area 窗口所在屏幕的工作区。
 * @returns 新边界；尺寸不小于新的最小尺寸，工作区放得下时完全落在工作区内。
 */
export function boundsForExtents(bounds: Rect, from: PanelExtents, to: PanelExtents, area: Rect): Rect {
  const minimum = minimumSizeFor(to);
  const wantedWidth = bounds.width + (to.left + to.right) - (from.left + from.right);
  const wantedHeight = bounds.height + to.bottom - from.bottom;
  const width = Math.min(Math.max(wantedWidth, minimum.width), Math.max(area.width, minimum.width));
  const height = Math.min(Math.max(wantedHeight, minimum.height), Math.max(area.height, minimum.height));
  const x = fit(bounds.x - (to.left - from.left), width, area.x, area.width);
  const y = fit(bounds.y, height, area.y, area.height);
  return { x, y, width, height };
}

/**
 * 从窗口边界扣掉面板占用的尺寸，得到面板全部关闭时的基础边界（保存到界面状态里）。
 * @param bounds 窗口现在的边界。
 * @param extents 展开的面板占用的尺寸。
 */
export function baseBounds(bounds: Rect, extents: PanelExtents): Rect {
  return {
    x: bounds.x + extents.left,
    y: bounds.y,
    width: Math.max(bounds.width - extents.left - extents.right, BASE_MIN_WIDTH),
    height: Math.max(bounds.height - extents.bottom, BASE_MIN_HEIGHT)
  };
}
