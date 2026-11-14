/*
 * ------------------------------------------------------------------------
 * 名称：shell-state.test.ts
 * 说明：界面状态校验的测试：没有保存或损坏时用默认值、单项不合格只替换那一项、窗口位置要么整体合格要么不用、布局修改的严格校验。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { DEFAULT_SHELL_STATE, parseLayoutPatch, parseShellState } from './shell-state';

describe('parseShellState', () => {
  it('没有保存过时：边栏与底部面板展开，右边栏折叠，窗口用默认位置', () => {
    for (const raw of [undefined, null, 5, 'x', [], {}]) {
      expect(parseShellState(raw)).toEqual({
        window: null,
        layout: { sideBarOpen: true, bottomPanelOpen: true, rightBarOpen: false, bottomPanelHeight: null }
      });
    }
  });

  it('完整合格的内容原样读出', () => {
    const state = {
      window: { x: -20, y: 30, width: 1280, height: 800, maximized: true },
      layout: { sideBarOpen: false, bottomPanelOpen: false, rightBarOpen: true, bottomPanelHeight: 240 }
    };
    expect(parseShellState(state)).toEqual(state);
  });

  it('单项不合格时只替换那一项，其他项照常读出', () => {
    const state = parseShellState({
      layout: { sideBarOpen: 'yes', bottomPanelOpen: false, rightBarOpen: true, bottomPanelHeight: 1.5 }
    });
    expect(state.layout).toEqual({ sideBarOpen: true, bottomPanelOpen: false, rightBarOpen: true, bottomPanelHeight: null });
  });

  it('窗口位置有一项不合格就整个不用', () => {
    expect(parseShellState({ window: { x: 0, y: 0, width: 0, height: 600, maximized: false } }).window).toBeNull();
    expect(parseShellState({ window: { x: 0, y: 0, width: 800, height: 600, maximized: 'no' } }).window).toBeNull();
    expect(parseShellState({ window: { x: 1.5, y: 0, width: 800, height: 600, maximized: false } }).window).toBeNull();
    expect(parseShellState({ window: { x: 0, y: 0, width: 800, height: 600 } }).window).toBeNull();
    expect(parseShellState({ window: 'x' }).window).toBeNull();
  });

  it('底部面板高度超出合理范围时用默认', () => {
    expect(parseShellState({ layout: { bottomPanelHeight: 0 } }).layout.bottomPanelHeight).toBeNull();
    expect(parseShellState({ layout: { bottomPanelHeight: 99_999 } }).layout.bottomPanelHeight).toBeNull();
    expect(parseShellState({ layout: { bottomPanelHeight: 300 } }).layout.bottomPanelHeight).toBe(300);
  });

  it('默认值不被改动', () => {
    parseShellState({ layout: { sideBarOpen: false } });
    expect(DEFAULT_SHELL_STATE.layout.sideBarOpen).toBe(true);
  });
});

describe('parseLayoutPatch', () => {
  it('合格的修改原样通过', () => {
    expect(parseLayoutPatch({ rightBarOpen: true })).toEqual({ rightBarOpen: true });
    expect(parseLayoutPatch({ bottomPanelHeight: 200, sideBarOpen: false })).toEqual({ bottomPanelHeight: 200, sideBarOpen: false });
    expect(parseLayoutPatch({ bottomPanelHeight: null })).toEqual({ bottomPanelHeight: null });
  });

  it('不是对象、空对象、含未知字段、值不合格时整条丢弃', () => {
    expect(parseLayoutPatch(undefined)).toBeNull();
    expect(parseLayoutPatch('x')).toBeNull();
    expect(parseLayoutPatch([])).toBeNull();
    expect(parseLayoutPatch({})).toBeNull();
    expect(parseLayoutPatch({ rightBarOpen: true, extra: 1 })).toBeNull();
    expect(parseLayoutPatch({ rightBarOpen: 'true' })).toBeNull();
    expect(parseLayoutPatch({ bottomPanelHeight: -1 })).toBeNull();
    expect(parseLayoutPatch({ bottomPanelHeight: 10.5 })).toBeNull();
    expect(parseLayoutPatch({ rightBarOpen: true, bottomPanelHeight: 'x' })).toBeNull();
  });

  it('原型链上的名字不算字段', () => {
    expect(parseLayoutPatch(JSON.parse('{"__proto__": {"a": 1}}'))).toBeNull();
    expect(parseLayoutPatch({ toString: true })).toBeNull();
  });
});
