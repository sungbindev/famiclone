/*
 * ------------------------------------------------------------------------
 * 名称：shell-state.test.ts
 * 说明：界面状态校验的测试：没有保存或损坏时用默认值、单项不合格只替换那一项、窗口位置要么整体合格要么不用、已选机型、旧文件里的面板高度被忽略、上次显示更新记录时的版本、布局修改的严格校验。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { DEFAULT_SHELL_STATE, parseLayoutPatch, parseShellState } from './shell-state';

describe('parseShellState', () => {
  it('没有保存过时：窗口用默认位置，没有选过机型', () => {
    for (const raw of [undefined, null, 5, 'x', [], {}]) {
      expect(parseShellState(raw)).toEqual({ window: null, layout: { selectedMachine: null, autoCollapsePanels: [] }, releaseNotesVersion: null });
    }
  });

  it('上次显示更新记录时的版本：写法合格才读出，其他用默认（没有记录）', () => {
    expect(parseShellState({ releaseNotesVersion: '0.2.0' }).releaseNotesVersion).toBe('0.2.0');
    for (const bad of ['', 'v0.2.0', '0.2.0 ', '0'.repeat(65), 2, null, ['0.2.0']]) {
      expect(parseShellState({ releaseNotesVersion: bad }).releaseNotesVersion).toBeNull();
    }
  });

  it('完整合格的内容原样读出', () => {
    const state = {
      window: { x: -20, y: 30 },
      layout: { selectedMachine: 'sb2000', autoCollapsePanels: ['runtime.panel', 'media.devices'] },
      releaseNotesVersion: '0.1.0'
    };
    expect(parseShellState(state)).toEqual(state);
  });

  it('旧文件里的窗口大小与是否最大化被忽略，只读出位置', () => {
    expect(parseShellState({ window: { x: 1, y: 2, width: 900, height: 600, maximized: true } }).window).toEqual({ x: 1, y: 2 });
  });

  it('旧文件里的面板展开状态与面板高度被忽略', () => {
    const state = parseShellState({ layout: { sideBarOpen: false, bottomPanelOpen: true, rightBarOpen: true, bottomPanelHeight: 200 } });
    expect(state.layout).toEqual({ selectedMachine: null, autoCollapsePanels: [] });
  });

  it('已选机型：小写字母开头的标识才读出，其他用默认（没选过）', () => {
    expect(parseShellState({ layout: { selectedMachine: 'bbk10' } }).layout.selectedMachine).toBe('bbk10');
    for (const bad of ['', 'Sb2000', '2000', 'a-b', 'x'.repeat(33), 5, true]) {
      expect(parseShellState({ layout: { selectedMachine: bad } }).layout.selectedMachine).toBeNull();
    }
  });

  it('单项不合格时只替换那一项，其他项照常读出', () => {
    const state = parseShellState({
      window: { x: 1, y: 2 },
      layout: { selectedMachine: 'Bad Id', autoCollapsePanels: ['runtime.panel'] }
    });
    expect(state.layout).toEqual({ selectedMachine: null, autoCollapsePanels: ['runtime.panel'] });
    expect(state.window).toEqual({ x: 1, y: 2 });
  });

  it('自动收缩的面板清单：写法合格、没有重复才读出，否则用默认（都关闭）', () => {
    expect(parseShellState({ layout: { autoCollapsePanels: ['runtime.panel'] } }).layout.autoCollapsePanels).toEqual(['runtime.panel']);
    for (const bad of ['runtime.panel', [5], ['Runtime.panel'], ['panel'], ['a.b', 'a.b'], Array.from({ length: 17 }, (_, index) => `m.p${index}`), null]) {
      expect(parseShellState({ layout: { autoCollapsePanels: bad } }).layout.autoCollapsePanels).toEqual([]);
    }
  });

  it('窗口位置有一项不合格就整个不用', () => {
    expect(parseShellState({ window: { x: 1.5, y: 0 } }).window).toBeNull();
    expect(parseShellState({ window: { x: 0, y: 'no' } }).window).toBeNull();
    expect(parseShellState({ window: { x: 0 } }).window).toBeNull();
    expect(parseShellState({ window: { x: 200_000, y: 0 } }).window).toBeNull();
    expect(parseShellState({ window: 'x' }).window).toBeNull();
  });

  it('默认值不被改动', () => {
    parseShellState({ layout: { selectedMachine: 'sb2000' } });
    expect(DEFAULT_SHELL_STATE.layout.selectedMachine).toBeNull();
  });
});

describe('parseLayoutPatch', () => {
  it('合格的修改原样通过', () => {
    expect(parseLayoutPatch({ selectedMachine: 'sb2000' })).toEqual({ selectedMachine: 'sb2000' });
    expect(parseLayoutPatch({ selectedMachine: null })).toEqual({ selectedMachine: null });
    expect(parseLayoutPatch({ autoCollapsePanels: ['runtime.panel'] })).toEqual({ autoCollapsePanels: ['runtime.panel'] });
    expect(parseLayoutPatch({ autoCollapsePanels: [] })).toEqual({ autoCollapsePanels: [] });
  });

  it('不是对象、空对象、含未知字段、值不合格时整条丢弃', () => {
    expect(parseLayoutPatch(undefined)).toBeNull();
    expect(parseLayoutPatch('x')).toBeNull();
    expect(parseLayoutPatch([])).toBeNull();
    expect(parseLayoutPatch({})).toBeNull();
    expect(parseLayoutPatch({ selectedMachine: 'sb2000', extra: 1 })).toBeNull();
    expect(parseLayoutPatch({ rightBarOpen: true })).toBeNull();
    expect(parseLayoutPatch({ bottomPanelHeight: 200 })).toBeNull();
    expect(parseLayoutPatch({ selectedMachine: 5 })).toBeNull();
    expect(parseLayoutPatch({ selectedMachine: 'Bad Id' })).toBeNull();
    expect(parseLayoutPatch({ autoCollapsePanels: ['runtime.panel', 'runtime.panel'] })).toBeNull();
    expect(parseLayoutPatch({ autoCollapsePanels: 'runtime.panel' })).toBeNull();
  });

  it('原型链上的名字不算字段', () => {
    expect(parseLayoutPatch(JSON.parse('{"__proto__": {"a": 1}}'))).toBeNull();
    expect(parseLayoutPatch({ toString: true })).toBeNull();
  });
});
