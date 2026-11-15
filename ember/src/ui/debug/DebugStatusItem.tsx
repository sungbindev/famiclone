/*
 * ------------------------------------------------------------------------
 * 名称：DebugStatusItem.tsx
 * 说明：调试服务登记到状态栏右侧（通知按钮左边）的项：调试服务开启时显示绿色文字“调试模式”，没有开启或还没读到状态时不显示；点击打开首选项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：端口、连接数放在悬停提示里；开启了但没有监听（如端口被占用）时文字仍是“调试模式”，用警告色，提示里写原因；
 *       点击执行命令 preferences.open（打开首选项对话框，不定位到“调试服务”分组），规则见 docs/05-界面.md 第 6 节。
 * ------------------------------------------------------------------------
 */

import { StatusBarItem } from '@rujianjs/ui-kit';
import { useSyncExternalStore } from 'react';
import { useShell } from '../shell/shell-context';
import type { DebugController } from './debug-controller';

/** 点击后执行的命令：打开首选项。 */
const OPEN_PREFERENCES_COMMAND = 'preferences.open';

/** 用户可见的文字。 */
const TEXT = {
  mode: '调试模式',
  address: (port: number) => `调试服务：ws://127.0.0.1:${port}`,
  connections: (count: number) => `，${count} 个连接`,
  failure: (reason: string) => `调试服务没有监听：${reason}`,
  notListening: '调试服务没有监听'
} as const;

/** 状态栏项的属性。 */
export interface DebugStatusItemProps {
  readonly controller: DebugController;
}

/** 调试服务的状态项；没有开启或还没读到状态时返回 null。 */
export function DebugStatusItem({ controller }: DebugStatusItemProps) {
  const state = useSyncExternalStore(controller.subscribe, controller.snapshot);
  const shell = useShell();
  if (state.kind !== 'ready' || !state.enabled) return null;
  const open = (): void => shell.runCommand(OPEN_PREFERENCES_COMMAND);
  if (!state.listening) return <StatusBarItem text={TEXT.mode} tone="warning" title={state.error === undefined ? TEXT.notListening : TEXT.failure(state.error)} onClick={open} />;
  const title = `${TEXT.address(state.port)}${state.connections > 0 ? TEXT.connections(state.connections) : ''}`;
  return <StatusBarItem text={TEXT.mode} tone="success" title={title} onClick={open} />;
}
