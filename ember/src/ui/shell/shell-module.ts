/*
 * ------------------------------------------------------------------------
 * 名称：shell-module.ts
 * 说明：外壳自己的登记项：命令与菜单项（文件→退出；视图→边栏、底部面板、右边栏、全屏及默认快捷键）、底部面板的“消息”页签。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：外壳自己也走登记表，所以快捷键冲突、标识前缀都按同样的规则检查；右边栏是否能打开由 right-bar.ts 决定。
 * ------------------------------------------------------------------------
 */

import { MessagesActions, MessagesPanel } from './MessagesPanel';
import { SHELL_NAMESPACE } from './build-registry';
import type { ShellCommandDefinition, ShellModule } from './shell-registry';

/** 外壳命令的标识。 */
export const SHELL_COMMANDS = {
  quit: `${SHELL_NAMESPACE}.quit`,
  toggleSideBar: `${SHELL_NAMESPACE}.toggleSideBar`,
  toggleBottomPanel: `${SHELL_NAMESPACE}.toggleBottomPanel`,
  toggleRightBar: `${SHELL_NAMESPACE}.toggleRightBar`,
  toggleFullScreen: `${SHELL_NAMESPACE}.toggleFullScreen`
} as const;

/** 底部面板“消息”页签的标识。 */
export const MESSAGES_PANEL_ID = `${SHELL_NAMESPACE}.messages`;

const TEXT = {
  quit: '退出',
  sideBar: '边栏',
  bottomPanel: '底部面板',
  rightBar: '右边栏',
  fullScreen: '全屏',
  messages: '消息'
} as const;

/** 外壳的命令。 */
const COMMANDS: readonly ShellCommandDefinition[] = [
  { id: SHELL_COMMANDS.quit, title: TEXT.quit, run: (shell) => shell.quit() },
  {
    id: SHELL_COMMANDS.toggleSideBar,
    title: TEXT.sideBar,
    defaultShortcut: 'Ctrl+B',
    run: (shell) => shell.toggleSideBar(),
    enabled: (view) => view.hasSideBar,
    checked: (view) => view.sideBarOpen
  },
  {
    id: SHELL_COMMANDS.toggleBottomPanel,
    title: TEXT.bottomPanel,
    defaultShortcut: 'Ctrl+J',
    run: (shell) => shell.toggleBottomPanel(),
    checked: (view) => view.bottomPanelOpen
  },
  {
    id: SHELL_COMMANDS.toggleRightBar,
    title: TEXT.rightBar,
    defaultShortcut: 'Ctrl+Alt+B',
    run: (shell) => shell.toggleRightBar(),
    enabled: (view) => view.canOpenRightBar,
    checked: (view) => view.rightBarOpen
  },
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
  menuItems: [
    { menu: 'file', command: SHELL_COMMANDS.quit, separatorBefore: true },
    { menu: 'view', command: SHELL_COMMANDS.toggleSideBar, separatorBefore: true },
    { menu: 'view', command: SHELL_COMMANDS.toggleBottomPanel },
    { menu: 'view', command: SHELL_COMMANDS.toggleRightBar },
    { menu: 'view', command: SHELL_COMMANDS.toggleFullScreen }
  ],
  bottomPanels: [{ id: MESSAGES_PANEL_ID, title: TEXT.messages, Component: MessagesPanel, Actions: MessagesActions }]
};
