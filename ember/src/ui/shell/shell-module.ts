/*
 * ------------------------------------------------------------------------
 * 名称：shell-module.ts
 * 说明：外壳自己的登记项：命令与菜单项（文件→退出）、全屏命令（F11，没有菜单项）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：外壳自己也走登记表，所以快捷键冲突、标识前缀都按同样的规则检查；底部面板没有快捷键与菜单，只经系统栏左侧的按钮展开。
 * ------------------------------------------------------------------------
 */

import { SHELL_NAMESPACE } from './build-registry';
import type { ShellCommandDefinition, ShellModule } from './shell-registry';

/** 外壳命令的标识。 */
export const SHELL_COMMANDS = {
  quit: `${SHELL_NAMESPACE}.quit`,
  toggleFullScreen: `${SHELL_NAMESPACE}.toggleFullScreen`
} as const;

const TEXT = {
  quit: '退出',
  fullScreen: '全屏'
} as const;

/** 外壳的命令。 */
const COMMANDS: readonly ShellCommandDefinition[] = [
  { id: SHELL_COMMANDS.quit, title: TEXT.quit, run: (shell) => shell.quit() },
  {
    id: SHELL_COMMANDS.toggleFullScreen,
    title: TEXT.fullScreen,
    defaultShortcut: 'F11',
    run: (shell) => shell.toggleFullScreen(),
    checked: (view) => view.fullScreen
  }
];

/** 外壳自己的登记项，排在渲染进程模块清单之前。 */
export const SHELL_MODULE: ShellModule = {
  name: SHELL_NAMESPACE,
  commands: COMMANDS,
  menuItems: [{ menu: 'file', command: SHELL_COMMANDS.quit, separatorBefore: true }]
};
