/*
 * ------------------------------------------------------------------------
 * 名称：MediaHost.tsx
 * 说明：介质模块的常驻组件：外壳启动后一直挂载，把预加载脚本的接口、外壳的刷新能力与组件库的通知接到设备面板的控制器上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：没有界面；让打印机输出一页的通知在设备面板没展开时也能收到。必须放在组件库根组件 UiProvider 之内（通知需要它）。
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
  useEffect(() => controller.attach(bridge, { notify, refresh: shell.refreshCommands }), [bridge, controller, shell, notify]);
  return null;
}
