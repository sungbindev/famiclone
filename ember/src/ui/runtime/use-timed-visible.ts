/*
 * ------------------------------------------------------------------------
 * 名称：use-timed-visible.ts
 * 说明：限时显示：条件成立后显示一段时间就自动消失，条件不再成立后再次成立时重新计时。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用于画面上的操作提示，提示不一直遮挡画面。
 * ------------------------------------------------------------------------
 */

import { useEffect, useState } from 'react';

/**
 * 条件成立后显示 durationMs 毫秒，之后消失；条件变为不成立后再次成立时重新计时。
 * @param wanted 是否需要显示。
 * @param durationMs 显示的毫秒数。
 * @returns 现在是否显示。
 */
export function useTimedVisible(wanted: boolean, durationMs: number): boolean {
  const [state, setState] = useState({ wanted, expired: false });
  // 条件变化时在渲染中重置计时状态（React 允许的“由属性推导状态”写法）。
  if (state.wanted !== wanted) setState({ wanted, expired: false });
  useEffect(() => {
    if (!wanted) return undefined;
    const timer = window.setTimeout(() => setState((current) => ({ ...current, expired: true })), durationMs);
    return () => window.clearTimeout(timer);
  }, [wanted, durationMs]);
  return wanted && !(state.wanted === wanted && state.expired);
}
