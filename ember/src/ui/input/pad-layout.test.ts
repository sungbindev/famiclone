/*
 * ------------------------------------------------------------------------
 * 名称：pad-layout.test.ts
 * 说明：手柄图版面的测试：10 个键各有一个位置、都在手柄外形里、互相不重叠、圆形键宽高相等、百分比换算正确。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：纯数据检查，不渲染界面。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { PAD_BUTTONS } from './input-model';
import { PAD_BODY, PAD_CANVAS, PAD_CROSS_CENTER, PAD_LAYOUT, percentOf, type PadKeyLayout } from './pad-layout';

/** 键占的矩形。 */
function boxOf(key: Pick<PadKeyLayout, 'cx' | 'cy' | 'width' | 'height'>) {
  return { left: key.cx - key.width / 2, top: key.cy - key.height / 2, right: key.cx + key.width / 2, bottom: key.cy + key.height / 2 };
}

describe('手柄图的版面', () => {
  it('手柄的 10 个键各有一个位置，没有多余的', () => {
    expect(PAD_LAYOUT.map((key) => key.id).toSorted()).toEqual(PAD_BUTTONS.map((button) => button.id).toSorted());
  });

  it('每个键都在手柄外形里，圆形键宽高相等', () => {
    for (const key of PAD_LAYOUT) {
      const box = boxOf(key);
      expect(box.left, key.id).toBeGreaterThanOrEqual(PAD_BODY.x);
      expect(box.top, key.id).toBeGreaterThanOrEqual(PAD_BODY.y);
      expect(box.right, key.id).toBeLessThanOrEqual(PAD_BODY.x + PAD_BODY.width);
      expect(box.bottom, key.id).toBeLessThanOrEqual(PAD_BODY.y + PAD_BODY.height);
      if (key.shape === 'round') expect(key.width, key.id).toBe(key.height);
    }
  });

  it('键之间不重叠；十字键的四个方向与中间的装饰块刚好相接', () => {
    for (const [index, first] of PAD_LAYOUT.entries()) {
      for (const second of PAD_LAYOUT.slice(index + 1)) {
        const one = boxOf(first);
        const other = boxOf(second);
        const apart = one.right <= other.left || other.right <= one.left || one.bottom <= other.top || other.bottom <= one.top;
        expect(apart, `${first.id} 与 ${second.id}`).toBe(true);
      }
    }
    const cross = boxOf({ cx: PAD_CROSS_CENTER.x + PAD_CROSS_CENTER.width / 2, cy: PAD_CROSS_CENTER.y + PAD_CROSS_CENTER.height / 2, ...PAD_CROSS_CENTER });
    const byId = (id: string) => boxOf(PAD_LAYOUT.find((key) => key.id === id) as PadKeyLayout);
    expect(byId('up').bottom).toBe(cross.top);
    expect(byId('down').top).toBe(cross.bottom);
    expect(byId('left').right).toBe(cross.left);
    expect(byId('right').left).toBe(cross.right);
  });

  it('百分比相对画布：矩形键有高度，圆形键的高度由宽高比决定', () => {
    const select = percentOf(PAD_LAYOUT.find((key) => key.id === 'select') as PadKeyLayout);
    expect(select.left).toBeCloseTo((232 / PAD_CANVAS.width) * 100);
    expect(select.top).toBeCloseTo((168 / PAD_CANVAS.height) * 100);
    expect(select.height).toBeCloseTo((34 / PAD_CANVAS.height) * 100);
    expect(percentOf(PAD_LAYOUT.find((key) => key.id === 'a') as PadKeyLayout).height).toBeUndefined();
  });
});
