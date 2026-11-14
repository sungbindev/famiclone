/*
 * ------------------------------------------------------------------------
 * 名称：message-log.test.ts
 * 说明：消息表的测试：追加带时间与级别、编号递增、默认取当前时间、清空、订阅与取消订阅、快照只在变化时更换、超出上限丢弃最早的。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { MAX_MESSAGES, MessageLog } from './message-log';

describe('MessageLog', () => {
  it('追加的消息带时间、级别、文字，编号递增', () => {
    const log = new MessageLog();
    const time = new Date(2026, 10, 14, 9, 30, 5);
    log.append({ level: 'info', text: '第一条', time });
    log.append({ level: 'error', text: '第二条', time });
    expect(log.snapshot()).toEqual([
      { id: 1, time, level: 'info', text: '第一条' },
      { id: 2, time, level: 'error', text: '第二条' }
    ]);
  });

  it('没有给时间时取当前时间', () => {
    vi.useFakeTimers();
    vi.setSystemTime(new Date(2026, 10, 14, 12, 0, 0));
    try {
      const log = new MessageLog();
      log.append({ level: 'warning', text: '提示' });
      expect(log.snapshot()[0]?.time.getTime()).toBe(new Date(2026, 10, 14, 12, 0, 0).getTime());
    } finally {
      vi.useRealTimers();
    }
  });

  it('清空后没有消息，编号继续递增，没有消息时清空不通知', () => {
    const log = new MessageLog();
    const listener = vi.fn();
    log.subscribe(listener);
    log.clear();
    expect(listener).not.toHaveBeenCalled();
    log.append({ level: 'info', text: 'a' });
    log.clear();
    log.append({ level: 'info', text: 'b' });
    expect(log.snapshot().map((message) => message.id)).toEqual([2]);
    expect(listener).toHaveBeenCalledTimes(3);
  });

  it('订阅者在变化时收到通知，取消订阅后不再收到', () => {
    const log = new MessageLog();
    const listener = vi.fn();
    const unsubscribe = log.subscribe(listener);
    log.append({ level: 'info', text: 'a' });
    expect(listener).toHaveBeenCalledTimes(1);
    unsubscribe();
    log.append({ level: 'info', text: 'b' });
    expect(listener).toHaveBeenCalledTimes(1);
  });

  it('快照只在变化时换成新数组', () => {
    const log = new MessageLog();
    const first = log.snapshot();
    expect(log.snapshot()).toBe(first);
    log.append({ level: 'info', text: 'a' });
    expect(log.snapshot()).not.toBe(first);
    expect(first).toEqual([]);
  });

  it('超出上限时丢弃最早的', () => {
    const log = new MessageLog();
    for (let index = 0; index < MAX_MESSAGES + 5; index += 1) log.append({ level: 'info', text: String(index) });
    const messages = log.snapshot();
    expect(messages).toHaveLength(MAX_MESSAGES);
    expect(messages[0]?.text).toBe('5');
    expect(messages[MAX_MESSAGES - 1]?.text).toBe(String(MAX_MESSAGES + 4));
  });
});
