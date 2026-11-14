/*
 * ------------------------------------------------------------------------
 * 名称：panel-extents.test.ts
 * 说明：面板占用尺寸校验的测试：合格的原样通过，缺项、多项、非整数、负数、超范围、不是对象时整条丢弃。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { NO_PANEL_EXTENTS, parsePanelExtents, samePanelExtents } from './panel-extents';

describe('parsePanelExtents', () => {
  it('合格的尺寸原样通过', () => {
    expect(parsePanelExtents({ left: 240, right: 0, bottom: 160 })).toEqual({ left: 240, right: 0, bottom: 160 });
    expect(parsePanelExtents({ left: 0, right: 0, bottom: 0 })).toEqual(NO_PANEL_EXTENTS);
  });

  it('不合格的整条丢弃', () => {
    const bad: unknown[] = [
      undefined,
      null,
      'x',
      5,
      [],
      {},
      { left: 1, right: 2 },
      { left: 1, right: 2, bottom: 3, extra: 4 },
      { left: 1.5, right: 0, bottom: 0 },
      { left: -1, right: 0, bottom: 0 },
      { left: 0, right: 10_001, bottom: 0 },
      { left: 0, right: 0, bottom: '1' },
      { left: Number.NaN, right: 0, bottom: 0 }
    ];
    for (const raw of bad) expect(parsePanelExtents(raw)).toBeNull();
  });
});

describe('samePanelExtents', () => {
  it('三项都相同才相同', () => {
    expect(samePanelExtents({ left: 1, right: 2, bottom: 3 }, { left: 1, right: 2, bottom: 3 })).toBe(true);
    expect(samePanelExtents({ left: 1, right: 2, bottom: 3 }, { left: 1, right: 2, bottom: 4 })).toBe(false);
  });
});
