/*
 * ------------------------------------------------------------------------
 * 名称：shell-context.tsx
 * 说明：外壳提供给页面、面板与状态项的上下文：外壳接口（useShell）与消息表（useShellMessages）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：模块只经这里的 Hook 与外壳协作，不引用外壳的界面代码；外壳接口在外壳的生命周期内保持不变，用作 Effect 的依赖是安全的。
 * ------------------------------------------------------------------------
 */

import { createContext, useContext, useSyncExternalStore } from 'react';
import type { MessageLog, ShellMessage } from './message-log';
import type { ShellApi } from './shell-registry';

/** 外壳接口的上下文。 */
export const ShellApiContext = createContext<ShellApi | null>(null);
/** 消息表的上下文。 */
export const MessageLogContext = createContext<MessageLog | null>(null);

/** 取外壳接口：打开页面、切换区域、追加消息、执行命令。 */
export function useShell(): ShellApi {
  const api = useContext(ShellApiContext);
  if (api === null) throw new Error('useShell 必须在外壳里使用');
  return api;
}

/** 取消息表与它的全部消息（消息变化时重新渲染），清空经 clear。 */
export function useShellMessages(): { readonly messages: readonly ShellMessage[]; readonly clear: () => void } {
  const log = useContext(MessageLogContext);
  if (log === null) throw new Error('useShellMessages 必须在外壳里使用');
  const messages = useSyncExternalStore(log.subscribe, log.snapshot);
  return { messages, clear: () => log.clear() };
}
