/*
 * ------------------------------------------------------------------------
 * 名称：use-timed-visible.test.ts
 * 说明：限时显示的测试：条件成立后显示到时间就消失；条件不成立时不显示；条件撤销后再次成立重新计时。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { act, renderHook } from '@testing-library/react';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { useTimedVisible } from './use-timed-visible';

beforeEach(() => vi.useFakeTimers());
afterEach(() => vi.useRealTimers());

describe('useTimedVisible', () => {
  it('条件不成立时不显示', () => {
    const { result } = renderHook(() => useTimedVisible(false, 1000));
    expect(result.current).toBe(false);
  });

  it('条件成立后显示，到时间消失，之后保持消失', () => {
    const { result } = renderHook(() => useTimedVisible(true, 1000));
    expect(result.current).toBe(true);
    act(() => vi.advanceTimersByTime(999));
    expect(result.current).toBe(true);
    act(() => vi.advanceTimersByTime(1));
    expect(result.current).toBe(false);
    act(() => vi.advanceTimersByTime(5000));
    expect(result.current).toBe(false);
  });

  it('条件撤销后再次成立，重新显示并重新计时', () => {
    const { result, rerender } = renderHook(({ wanted }) => useTimedVisible(wanted, 1000), { initialProps: { wanted: true } });
    act(() => vi.advanceTimersByTime(1000));
    expect(result.current).toBe(false);
    rerender({ wanted: false });
    expect(result.current).toBe(false);
    rerender({ wanted: true });
    expect(result.current).toBe(true);
    act(() => vi.advanceTimersByTime(1000));
    expect(result.current).toBe(false);
  });
});
