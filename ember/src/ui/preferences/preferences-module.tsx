/*
 * ------------------------------------------------------------------------
 * 名称：preferences-module.tsx
 * 说明：首选项模块在外壳里的登记项：“首选项”对话框、打开它的命令（Ctrl+,）与“设置”菜单里的菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：对话框是独立的系统窗口，同一时间只开一个；规则见 docs/05-界面.md 第 3、4.5、5 节与 docs/06 第 3 节。
 * ------------------------------------------------------------------------
 */

import type { ShellModule } from '../shell/shell-registry';
import { PreferencesPage } from './PreferencesPage';

/** “首选项”对话框的标识。 */
const DIALOG_ID = 'preferences.preferences';

/** 命令的标识。 */
export const PREFERENCES_COMMANDS = { open: 'preferences.open' } as const;

/** 对话框内容区的初始尺寸与最小尺寸（像素）。 */
const DIALOG_SIZE = { width: 720, height: 520, minWidth: 560, minHeight: 360 } as const;

/** 用户可见的文字。 */
const TEXT = { title: '首选项' } as const;

/** 首选项模块。 */
export const PREFERENCES_MODULE: ShellModule = {
  name: 'preferences',
  dialogs: [{ id: DIALOG_ID, title: TEXT.title, Component: PreferencesPage, ...DIALOG_SIZE }],
  commands: [{ id: PREFERENCES_COMMANDS.open, title: TEXT.title, defaultShortcut: 'Ctrl+,', run: (shell) => shell.openDialog(DIALOG_ID) }],
  menuItems: [{ menu: 'settings', command: PREFERENCES_COMMANDS.open }]
};
