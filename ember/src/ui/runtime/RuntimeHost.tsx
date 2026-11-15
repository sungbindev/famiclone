/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeHost.tsx
 * 说明：运行模块的常驻组件：外壳启动后一直挂载，把预加载脚本的接口、外壳的命令刷新能力与组件库的通知接到运行模块的控制器上。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：没有界面；让状态栏与菜单在面板没展开、显示区没显示时也有最新的机器状态。必须放在组件库根组件 UiProvider 之内（通知需要它）。
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
    () => controller.attach(bridge, { notify, refreshCommands: shell.refreshCommands }),
    [bridge, controller, shell, notify]
  );
  return null;
}
