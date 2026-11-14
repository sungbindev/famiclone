/*
 * ------------------------------------------------------------------------
 * 名称：debug-module.tsx
 * 说明：调试服务模块在外壳里的登记项：状态栏右侧的服务状态项，以及把状态控制器接到预加载接口上的常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：控制器由 bootstrap/renderer/modules/debug.ts 创建并传入；没有页面、菜单与命令（调试服务的开关、端口与令牌在首选项里，见 docs/05-界面.md 第 4.5 节）。
 * ------------------------------------------------------------------------
 */

import { useEffect } from 'react';
import type { ShellModule, ShellPageProps } from '../shell/shell-registry';
import type { DebugController } from './debug-controller';
import { DebugStatusItem } from './DebugStatusItem';

/** 常驻组件的属性。 */
interface DebugHostProps extends ShellPageProps {
  readonly controller: DebugController;
}

/** 常驻组件：外壳启动后一直挂载，让状态栏项有最新的服务状态。 */
function DebugHost({ bridge, controller }: DebugHostProps) {
  useEffect(() => controller.attach(bridge), [bridge, controller]);
  return null;
}

/**
 * 创建调试服务模块的登记项。
 * @param controller 状态控制器，状态栏项与常驻组件共用。
 */
export function createDebugModule(controller: DebugController): ShellModule {
  return {
    name: 'debug',
    statusItems: [{ id: 'debug.service', align: 'end', Component: () => <DebugStatusItem controller={controller} /> }],
    Host: (props: ShellPageProps) => <DebugHost {...props} controller={controller} />
  };
}
