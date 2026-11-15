/*
 * ------------------------------------------------------------------------
 * 名称：input-module.tsx
 * 说明：输入模块在外壳里的登记项：“手柄设置”对话框、打开它的命令与“设置”菜单里的菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：对话框是独立的系统窗口，同一时间只开一个；规则见 docs/05-界面.md 第 3、4.6、5 节与 docs/06 第 3 节。
 * ------------------------------------------------------------------------
 */

import type { ShellModule } from '../shell/shell-registry';
import { GamepadSettings } from './GamepadSettings';

/** “手柄设置”对话框的标识。 */
const DIALOG_ID = 'input.gamepadSettings';

/** 命令的标识。 */
export const INPUT_COMMANDS = { open: 'input.openGamepadSettings' } as const;

/** 对话框内容区的初始尺寸与最小尺寸（像素）。 */
const DIALOG_SIZE = { width: 640, height: 600, minWidth: 520, minHeight: 360 } as const;

/** 用户可见的文字。 */
const TEXT = { title: '手柄设置' } as const;

/** 输入模块。 */
export const INPUT_MODULE: ShellModule = {
  name: 'input',
  dialogs: [{ id: DIALOG_ID, title: TEXT.title, Component: GamepadSettings, ...DIALOG_SIZE }],
  commands: [{ id: INPUT_COMMANDS.open, title: TEXT.title, run: (shell) => shell.openDialog(DIALOG_ID) }],
  menuItems: [{ menu: 'settings', command: INPUT_COMMANDS.open }]
};
