/*
 * ------------------------------------------------------------------------
 * 名称：tab-state.test.ts
 * 说明：标签页状态的测试：单例页面重复打开只聚焦、非单例页面每次新开、切换、关闭当前标签激活右侧再左侧、关闭其他标签不改变当前、关闭最后一个标签。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { activateTab, closeTab, EMPTY_TABS, openTab } from './tab-state';

const A = { id: 'a.page' };
const B = { id: 'b.page' };
const C = { id: 'c.page' };

describe('openTab', () => {
  it('第一次打开追加标签并激活', () => {
    const state = openTab(EMPTY_TABS, A);
    expect(state.tabs).toEqual([{ id: 'a.page', pageId: 'a.page' }]);
    expect(state.activeId).toBe('a.page');
  });

  it('单例页面重复打开只聚焦已有标签，不新增', () => {
    let state = openTab(EMPTY_TABS, A);
    state = openTab(state, B);
    state = openTab(state, A);
    expect(state.tabs.map((tab) => tab.id)).toEqual(['a.page', 'b.page']);
    expect(state.activeId).toBe('a.page');
  });

  it('非单例页面每次打开都新增一个标签', () => {
    const page = { id: 'a.doc', singleton: false };
    let state = openTab(EMPTY_TABS, page);
    state = openTab(state, page);
    expect(state.tabs.map((tab) => tab.id)).toEqual(['a.doc#1', 'a.doc#2']);
    expect(state.tabs.every((tab) => tab.pageId === 'a.doc')).toBe(true);
    expect(state.activeId).toBe('a.doc#2');
  });
});

describe('activateTab', () => {
  it('切换到已有标签；标签不存在时不变', () => {
    const state = openTab(openTab(EMPTY_TABS, A), B);
    expect(activateTab(state, 'a.page').activeId).toBe('a.page');
    expect(activateTab(state, '没有').activeId).toBe('b.page');
  });
});

describe('closeTab', () => {
  const three = openTab(openTab(openTab(EMPTY_TABS, A), B), C);

  it('关闭当前标签时激活右侧的标签', () => {
    const state = closeTab(activateTab(three, 'b.page'), 'b.page');
    expect(state.tabs.map((tab) => tab.id)).toEqual(['a.page', 'c.page']);
    expect(state.activeId).toBe('c.page');
  });

  it('右侧没有标签时激活左侧的标签', () => {
    const state = closeTab(three, 'c.page');
    expect(state.activeId).toBe('b.page');
  });

  it('关闭的不是当前标签时当前标签不变', () => {
    const state = closeTab(three, 'a.page');
    expect(state.activeId).toBe('c.page');
    expect(state.tabs).toHaveLength(2);
  });

  it('关闭最后一个标签后没有当前标签', () => {
    const state = closeTab(openTab(EMPTY_TABS, A), 'a.page');
    expect(state.tabs).toEqual([]);
    expect(state.activeId).toBeNull();
  });

  it('标签不存在时不变', () => {
    expect(closeTab(three, '没有')).toBe(three);
  });
});
