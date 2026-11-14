/*
 * ------------------------------------------------------------------------
 * 名称：message-log.ts
 * 说明：底部面板“消息”页签的消息表：追加（时间、级别、文字）、清空、订阅变化；保留最近的若干条。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：外部存储，配合 useSyncExternalStore 使用；快照在内容变化时换成新数组；只存在于界面内存里，不写盘。
 * ------------------------------------------------------------------------
 */

import type { ShellMessageInput, ShellMessageLevel } from './shell-registry';

/** 最多保留的消息条数，超出时丢弃最早的。 */
export const MAX_MESSAGES = 1000;

/** 一条消息。 */
export interface ShellMessage {
  /** 递增的编号，用作列表的键。 */
  readonly id: number;
  readonly time: Date;
  readonly level: ShellMessageLevel;
  readonly text: string;
}

/** 消息表。 */
export class MessageLog {
  private messages: readonly ShellMessage[] = [];
  private nextId = 1;
  private readonly listeners = new Set<() => void>();

  /** 订阅变化；返回取消订阅的函数。 */
  readonly subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  };

  /** 当前的全部消息，最早的在前；内容变化时返回新数组。 */
  readonly snapshot = (): readonly ShellMessage[] => this.messages;

  /** 追加一条消息。 */
  append(input: ShellMessageInput): void {
    const message: ShellMessage = { id: this.nextId, time: input.time ?? new Date(), level: input.level, text: input.text };
    this.nextId += 1;
    const next = [...this.messages, message];
    this.messages = next.length > MAX_MESSAGES ? next.slice(next.length - MAX_MESSAGES) : next;
    this.emit();
  }

  /** 清空全部消息。 */
  clear(): void {
    if (this.messages.length === 0) return;
    this.messages = [];
    this.emit();
  }

  private emit(): void {
    for (const listener of this.listeners) listener();
  }
}
