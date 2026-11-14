/*
 * ------------------------------------------------------------------------
 * 名称：screen-layout.test.ts
 * 说明：画面版面计算的测试：两种比例、标准帧的显示比例、两种缩放方式、居中、放不下与无效尺寸。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { computeScreenRect, DEFAULT_SCREEN_ASPECT, displayAspectRatio, nominalScreenRatio } from './screen-layout';

describe('displayAspectRatio', () => {
  it('4:3 固定为 4/3，8:7 按像素宽高比算', () => {
    expect(displayAspectRatio('4:3', 256, 240)).toBeCloseTo(4 / 3, 10);
    expect(displayAspectRatio('8:7', 256, 240)).toBeCloseTo((256 * 8) / 7 / 240, 10);
    expect(displayAspectRatio('8:7', 256, 224)).toBeCloseTo((256 * 8) / 7 / 224, 10);
  });
});

describe('nominalScreenRatio', () => {
  it('默认的画面比例是 8:7，标准帧的显示宽高比为 128:105', () => {
    expect(DEFAULT_SCREEN_ASPECT).toBe('8:7');
    expect(nominalScreenRatio()).toBeCloseTo(128 / 105, 10);
  });

  it('可以指定画面比例', () => {
    expect(nominalScreenRatio('4:3')).toBeCloseTo(4 / 3, 10);
  });
});

describe('computeScreenRect：适应窗口', () => {
  it('画布比画面宽时按高度放大，左右留黑边并居中', () => {
    expect(computeScreenRect(1600, 600, 256, 240, '4:3', 'fit')).toEqual({ x: 400, y: 0, width: 800, height: 600 });
  });

  it('画布比画面窄时按宽度放大，上下留黑边并居中', () => {
    expect(computeScreenRect(800, 900, 256, 240, '4:3', 'fit')).toEqual({ x: 0, y: 150, width: 800, height: 600 });
  });

  it('8:7 的像素宽高比使画面比 4:3 窄', () => {
    const rect = computeScreenRect(2000, 480, 256, 240, '8:7', 'fit');
    expect(rect.height).toBe(480);
    expect(rect.width).toBe(Math.round((480 * 256 * 8) / 7 / 240));
    expect(rect.width).toBeLessThan(640);
    expect(rect.x).toBe(Math.floor((2000 - rect.width) / 2));
  });

  it('比画面小的画布也按比例缩小，不超出画布', () => {
    const rect = computeScreenRect(100, 100, 256, 240, '4:3', 'fit');
    expect(rect).toEqual({ x: 0, y: 12, width: 100, height: 75 });
  });
});

describe('computeScreenRect：整数倍', () => {
  it('取放得下的最大整数倍（垂直方向）', () => {
    const rect = computeScreenRect(1920, 1080, 256, 240, '4:3', 'integer');
    expect(rect.height).toBe(960);
    expect(rect.width).toBe(1280);
    expect(rect.x).toBe(320);
    expect(rect.y).toBe(60);
  });

  it('宽度先放不下时按宽度限制整数倍', () => {
    const rect = computeScreenRect(700, 2000, 256, 240, '4:3', 'integer');
    expect(rect.height).toBe(480);
    expect(rect.width).toBe(640);
  });

  it('刚好放得下一个整数倍时不放大也不缩小', () => {
    expect(computeScreenRect(1280, 960, 256, 240, '4:3', 'integer')).toEqual({ x: 0, y: 0, width: 1280, height: 960 });
  });

  it('一倍都放不下时退回适应窗口', () => {
    const integer = computeScreenRect(200, 150, 256, 240, '4:3', 'integer');
    expect(integer).toEqual(computeScreenRect(200, 150, 256, 240, '4:3', 'fit'));
  });

  it('8:7 的整数倍：高度是帧高度的整数倍，宽度按像素宽高比', () => {
    const rect = computeScreenRect(1280, 720, 256, 240, '8:7', 'integer');
    expect(rect.height).toBe(720);
    expect(rect.width).toBe(Math.round((720 * 256 * 8) / 7 / 240));
  });
});

describe('computeScreenRect：无效尺寸', () => {
  it('画布或帧尺寸为 0 或负数时返回空矩形', () => {
    const empty = { x: 0, y: 0, width: 0, height: 0 };
    expect(computeScreenRect(0, 100, 256, 240, '4:3', 'fit')).toEqual(empty);
    expect(computeScreenRect(100, 0, 256, 240, '4:3', 'fit')).toEqual(empty);
    expect(computeScreenRect(100, 100, 0, 240, '4:3', 'integer')).toEqual(empty);
    expect(computeScreenRect(-1, 100, 256, 240, '4:3', 'fit')).toEqual(empty);
    expect(computeScreenRect(Number.NaN, 100, 256, 240, '4:3', 'fit')).toEqual(empty);
  });
});
