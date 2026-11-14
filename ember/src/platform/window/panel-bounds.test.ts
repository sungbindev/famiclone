/*
 * ------------------------------------------------------------------------
 * 名称：panel-bounds.test.ts
 * 说明：窗口随面板伸缩计算的测试：最小尺寸、各面板向外伸缩的方向、工作区放不下时的平移与缩小、保存用的基础边界。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { describe, expect, it } from 'vitest';
import { BASE_MIN_HEIGHT, BASE_MIN_WIDTH, baseBounds, boundsForExtents, minimumSizeFor, NO_PANEL_EXTENTS } from './panel-bounds';

const AREA = { x: 0, y: 0, width: 1920, height: 1040 };
const WINDOW = { x: 400, y: 100, width: 1000, height: 700 };

describe('minimumSizeFor', () => {
  it('面板全部关闭时是基础最小尺寸', () => {
    expect(minimumSizeFor(NO_PANEL_EXTENTS)).toEqual({ width: BASE_MIN_WIDTH, height: BASE_MIN_HEIGHT });
  });

  it('展开的面板加到最小尺寸上', () => {
    expect(minimumSizeFor({ left: 240, right: 340, bottom: 160 })).toEqual({ width: BASE_MIN_WIDTH + 580, height: BASE_MIN_HEIGHT + 160 });
  });
});

describe('boundsForExtents', () => {
  it('边栏展开时窗口向左伸，右边缘不动', () => {
    const next = boundsForExtents(WINDOW, NO_PANEL_EXTENTS, { left: 240, right: 0, bottom: 0 }, AREA);
    expect(next).toEqual({ x: 160, y: 100, width: 1240, height: 700 });
  });

  it('右边栏展开时窗口向右伸，左边缘不动', () => {
    const next = boundsForExtents(WINDOW, NO_PANEL_EXTENTS, { left: 0, right: 340, bottom: 0 }, AREA);
    expect(next).toEqual({ x: 400, y: 100, width: 1340, height: 700 });
  });

  it('底部面板展开时窗口向下伸，上边缘不动', () => {
    const next = boundsForExtents(WINDOW, NO_PANEL_EXTENTS, { left: 0, right: 0, bottom: 160 }, AREA);
    expect(next).toEqual({ x: 400, y: 100, width: 1000, height: 860 });
  });

  it('折叠面板时按差值缩回', () => {
    const open = { left: 240, right: 340, bottom: 160 };
    const expanded = boundsForExtents(WINDOW, NO_PANEL_EXTENTS, open, AREA);
    expect(boundsForExtents(expanded, open, NO_PANEL_EXTENTS, AREA)).toEqual(WINDOW);
  });

  it('拖动面板的宽度时只伸缩变化的部分', () => {
    const next = boundsForExtents({ x: 160, y: 100, width: 1240, height: 700 }, { left: 240, right: 0, bottom: 0 }, { left: 300, right: 0, bottom: 0 }, AREA);
    expect(next).toEqual({ x: 100, y: 100, width: 1300, height: 700 });
  });

  it('向左伸出工作区时整体平移进工作区', () => {
    const next = boundsForExtents({ x: 100, y: 100, width: 1000, height: 700 }, NO_PANEL_EXTENTS, { left: 240, right: 0, bottom: 0 }, AREA);
    expect(next).toEqual({ x: 0, y: 100, width: 1240, height: 700 });
  });

  it('向下伸出工作区时整体上移', () => {
    const next = boundsForExtents({ x: 400, y: 500, width: 1000, height: 500 }, NO_PANEL_EXTENTS, { left: 0, right: 0, bottom: 160 }, AREA);
    expect(next).toEqual({ x: 400, y: 380, width: 1000, height: 660 });
  });

  it('工作区放不下时缩到工作区大小', () => {
    const next = boundsForExtents({ x: 0, y: 0, width: 1800, height: 1000 }, NO_PANEL_EXTENTS, { left: 240, right: 340, bottom: 160 }, AREA);
    expect(next).toEqual({ x: 0, y: 0, width: 1920, height: 1040 });
  });

  it('窗口比新的最小尺寸小时放大到最小尺寸', () => {
    const next = boundsForExtents({ x: 400, y: 100, width: 500, height: 300 }, { left: 240, right: 0, bottom: 0 }, NO_PANEL_EXTENTS, AREA);
    expect(next.width).toBe(BASE_MIN_WIDTH);
    expect(next.height).toBe(BASE_MIN_HEIGHT);
  });
});

describe('baseBounds', () => {
  it('扣掉面板占用的尺寸，边栏占用的部分让出左边缘', () => {
    expect(baseBounds({ x: 160, y: 100, width: 1580, height: 860 }, { left: 240, right: 340, bottom: 160 })).toEqual(WINDOW);
  });

  it('扣完比基础最小尺寸小时取最小尺寸', () => {
    const base = baseBounds({ x: 0, y: 0, width: 800, height: 600 }, { left: 240, right: 340, bottom: 160 });
    expect(base.width).toBe(BASE_MIN_WIDTH);
    expect(base.height).toBe(BASE_MIN_HEIGHT);
  });
});
