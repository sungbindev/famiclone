/*
 * ------------------------------------------------------------------------
 * 名称：panel-extents.ts
 * 说明：展开的面板占用的尺寸（边栏与右边栏的宽度、底部面板的高度）的类型与校验。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：界面把它发给主进程，主进程据此伸缩窗口并设置最小尺寸；渲染进程发来的值先经 parsePanelExtents 校验，含未知字段或不合格的值时整条丢弃。
 * ------------------------------------------------------------------------
 */

/** 展开的面板占用的尺寸（像素），折叠的面板为 0。 */
export interface PanelExtents {
  /** 边栏的宽度（在页面区左侧）。 */
  readonly left: number;
  /** 右边栏的宽度。 */
  readonly right: number;
  /** 底部面板的高度。 */
  readonly bottom: number;
}

/** 全部面板都折叠。 */
export const NO_PANEL_EXTENTS: PanelExtents = { left: 0, right: 0, bottom: 0 };

/** 单个面板尺寸的合理上限。 */
const MAX_EXTENT = 10_000;

/** 两个尺寸是否相同。 */
export function samePanelExtents(a: PanelExtents, b: PanelExtents): boolean {
  return a.left === b.left && a.right === b.right && a.bottom === b.bottom;
}

/**
 * 校验渲染进程发来的面板尺寸：只能含 left、right、bottom 三项，都是 0 到上限之间的整数。
 * @param raw 收到的内容。
 * @returns 合格的尺寸；不合格时为 null。
 */
export function parsePanelExtents(raw: unknown): PanelExtents | null {
  if (typeof raw !== 'object' || raw === null || Array.isArray(raw)) return null;
  const record = raw as Record<string, unknown>;
  const keys = Object.keys(record);
  if (keys.length !== 3 || !keys.every((key) => key === 'left' || key === 'right' || key === 'bottom')) return null;
  const { left, right, bottom } = record;
  for (const value of [left, right, bottom]) {
    if (typeof value !== 'number' || !Number.isInteger(value) || value < 0 || value > MAX_EXTENT) return null;
  }
  return { left: left as number, right: right as number, bottom: bottom as number };
}
