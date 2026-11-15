/*
 * ------------------------------------------------------------------------
 * 名称：MediaHost.tsx
 * 说明：介质模块的常驻组件：外壳启动后一直挂载，把预加载脚本的接口、外壳的消息与组件库的通知接到设备边栏的控制器上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：没有界面；让打印机输出一页的通知与消息在设备边栏没打开时也能收到。必须放在组件库根组件 UiProvider 之内（通知需要它）。
 * ------------------------------------------------------------------------
 */

import { useNotify } from '@rujianjs/ui-kit';
import { useEffect } from 'react';
import { useShell } from '../shell/shell-context';
import type { ShellPageProps } from '../shell/shell-registry';
import type { MediaController } from './media-controller';

/** 常驻组件的属性。 */
export interface MediaHostProps extends ShellPageProps {
  readonly controller: MediaController;
}

/**
 * 介质模块的常驻组件。
 * @param props 属性。
 */
export function MediaHost({ bridge, controller }: MediaHostProps) {
  const shell = useShell();
  const notify = useNotify();
  useEffect(() => controller.attach(bridge, { addMessage: shell.addMessage, notify, refresh: shell.refreshCommands }), [bridge, controller, shell, notify]);
  return null;
}
