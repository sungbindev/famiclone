/*
 * ------------------------------------------------------------------------
 * 名称：mappers-module.tsx
 * 说明：mapper 插件模块在外壳里的登记项：“mapper 插件”对话框、打开它的命令与“设置”菜单里的菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：对话框是独立的系统窗口，同一时间只开一个；规则见 docs/05-界面.md 第 3、4.4、5 节与 docs/06 第 3 节。
 * ------------------------------------------------------------------------
 */

import type { ShellModule } from '../shell/shell-registry';
import { MappersDialog } from './MappersDialog';

/** “mapper 插件”对话框的标识。 */
const DIALOG_ID = 'mappers.plugins';

/** 命令的标识。 */
export const MAPPERS_COMMANDS = { open: 'mappers.open' } as const;

/** 对话框内容区的初始尺寸与最小尺寸（像素）。 */
const DIALOG_SIZE = { width: 760, height: 560, minWidth: 560, minHeight: 360 } as const;

/** 用户可见的文字。 */
const TEXT = { title: 'mapper 插件' } as const;

/** mapper 插件模块。 */
export const MAPPERS_MODULE: ShellModule = {
  name: 'mappers',
  dialogs: [{ id: DIALOG_ID, title: TEXT.title, Component: MappersDialog, ...DIALOG_SIZE }],
  commands: [{ id: MAPPERS_COMMANDS.open, title: TEXT.title, run: (shell) => shell.openDialog(DIALOG_ID) }],
  menuItems: [{ menu: 'settings', command: MAPPERS_COMMANDS.open }]
};
