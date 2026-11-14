/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeHost.tsx
 * 说明：运行模块的常驻组件：外壳启动后一直挂载，把预加载脚本的接口、外壳的消息与命令刷新能力、组件库的通知接到运行页的控制器上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：没有界面；让边栏、状态栏与菜单在边栏没打开、页面没显示时也有最新的机器状态，并让“消息”持续收到运行事件。必须放在组件库根组件 UiProvider 之内（通知需要它）。
 * ------------------------------------------------------------------------
 */

import { useNotify } from '@rujianjs/ui-kit';
import { useEffect } from 'react';
import { useShell } from '../shell/shell-context';
import type { ShellPageProps } from '../shell/shell-registry';
import type { RuntimeController } from './runtime-controller';

/** 常驻组件的属性。 */
export interface RuntimeHostProps extends ShellPageProps {
  readonly controller: RuntimeController;
}

/**
 * 运行模块的常驻组件。
 * @param props 属性。
 */
export function RuntimeHost({ bridge, controller }: RuntimeHostProps) {
  const shell = useShell();
  const notify = useNotify();
  useEffect(
    () => controller.attach(bridge, { addMessage: shell.addMessage, notify, refreshCommands: shell.refreshCommands }),
    [bridge, controller, shell, notify]
  );
  return null;
}
