/*
 * ------------------------------------------------------------------------
 * 名称：DebugStatusItem.tsx
 * 说明：调试服务登记到状态栏右侧的项：服务开启时显示端口与连接数，没有监听时显示警告，没有开启时显示“调试已关闭”；点击打开首选项；还没读到状态时不显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：状态点：有连接为绿色，监听中没有连接或已关闭为灰色，开启了但没有监听为黄色，旁边始终有文字；点击执行命令 preferences.open（打开首选项页，不定位到“调试服务”分组），规则见 docs/05-界面.md 第 6 节。
 * ------------------------------------------------------------------------
 */

import { StatusBarItem, type StatusBarDot } from '@rujianjs/ui-kit';
import { useSyncExternalStore } from 'react';
import { useShell } from '../shell/shell-context';
import type { DebugController } from './debug-controller';

/** 点击后执行的命令：打开首选项页。 */
const OPEN_PREFERENCES_COMMAND = 'preferences.open';

/** 用户可见的文字。 */
const TEXT = {
  service: '调试服务',
  closed: '调试已关闭',
  closedHint: '调试服务已关闭，点击打开首选项',
  notListening: '调试服务未监听',
  address: (port: number) => `调试服务：ws://127.0.0.1:${port}`,
  failure: (reason: string) => `调试服务没有监听：${reason}`,
  connections: (count: number) => `（${count} 个连接）`
} as const;

/** 状态栏项的属性。 */
export interface DebugStatusItemProps {
  readonly controller: DebugController;
}

/** 调试服务的状态项；还没读到状态时返回 null。 */
export function DebugStatusItem({ controller }: DebugStatusItemProps) {
  const state = useSyncExternalStore(controller.subscribe, controller.snapshot);
  const shell = useShell();
  if (state.kind !== 'ready') return null;
  const open = (): void => shell.runCommand(OPEN_PREFERENCES_COMMAND);
  if (!state.enabled) return <StatusBarItem text={TEXT.closed} dot="neutral" title={TEXT.closedHint} onClick={open} />;
  if (!state.listening) {
    const dot: StatusBarDot = 'warning';
    return <StatusBarItem text={TEXT.notListening} dot={dot} title={state.error === undefined ? TEXT.service : TEXT.failure(state.error)} onClick={open} />;
  }
  const dot: StatusBarDot = state.connections > 0 ? 'success' : 'neutral';
  const text = `${TEXT.service} ${state.port}${state.connections > 0 ? TEXT.connections(state.connections) : ''}`;
  return <StatusBarItem text={text} dot={dot} title={TEXT.address(state.port)} onClick={open} />;
}
