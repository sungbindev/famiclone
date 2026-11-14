/*
 * ------------------------------------------------------------------------
 * 名称：tab-state.ts
 * 说明：标签页的状态与变化：打开（单例页面聚焦已有标签，非单例页面每次新开一个）、切换、关闭（关闭当前标签时激活右侧的标签，没有时激活左侧的）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯函数，不保存；启动时的标签由外壳按登记的 openOnStart 打开。
 * ------------------------------------------------------------------------
 */

/** 一个标签。 */
export interface ShellTab {
  /** 标签标识：单例页面等于页面标识，非单例页面是“页面标识#序号”。 */
  readonly id: string;
  readonly pageId: string;
}

/** 标签页状态。 */
export interface TabState {
  readonly tabs: readonly ShellTab[];
  readonly activeId: string | null;
  /** 下一个非单例标签的序号。 */
  readonly nextSerial: number;
}

/** 没有标签的状态。 */
export const EMPTY_TABS: TabState = { tabs: [], activeId: null, nextSerial: 1 };

/**
 * 打开页面：单例页面已打开时切换过去，否则追加一个标签并激活。
 * @param state 当前状态。
 * @param page 页面的标识与是否单例（默认单例）。
 */
export function openTab(state: TabState, page: { readonly id: string; readonly singleton?: boolean }): TabState {
  if (page.singleton !== false) {
    if (state.tabs.some((tab) => tab.id === page.id)) return { ...state, activeId: page.id };
    return { ...state, tabs: [...state.tabs, { id: page.id, pageId: page.id }], activeId: page.id };
  }
  const id = `${page.id}#${state.nextSerial}`;
  return { tabs: [...state.tabs, { id, pageId: page.id }], activeId: id, nextSerial: state.nextSerial + 1 };
}

/** 切换到指定标签；标签不存在时不变。 */
export function activateTab(state: TabState, id: string): TabState {
  return state.tabs.some((tab) => tab.id === id) ? { ...state, activeId: id } : state;
}

/** 关闭标签；关闭的是当前标签时激活右侧的标签，没有时激活左侧的；标签不存在时不变。 */
export function closeTab(state: TabState, id: string): TabState {
  const index = state.tabs.findIndex((tab) => tab.id === id);
  if (index < 0) return state;
  const tabs = state.tabs.filter((tab) => tab.id !== id);
  if (state.activeId !== id) return { ...state, tabs };
  return { ...state, tabs, activeId: (tabs[index] ?? tabs[index - 1])?.id ?? null };
}
