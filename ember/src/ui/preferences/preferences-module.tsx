/*
 * ------------------------------------------------------------------------
 * 名称：preferences-module.tsx
 * 说明：首选项模块在外壳里的登记项：“首选项”页、打开它的命令（Ctrl+,）与“设置”菜单里的菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：页面没有边栏（外壳自动关闭边栏）；规则见 docs/05-界面.md 第 3、4.5、5 节与 docs/06 第 3 节。
 * ------------------------------------------------------------------------
 */

import { iconSettings } from '@rujianjs/ui-kit';
import type { ShellModule } from '../shell/shell-registry';
import { PreferencesPage } from './PreferencesPage';

/** “首选项”页的标识。 */
const PAGE_ID = 'preferences.preferences';

/** 命令的标识。 */
export const PREFERENCES_COMMANDS = { open: 'preferences.open' } as const;

/** 用户可见的文字。 */
const TEXT = { title: '首选项' } as const;

/** 首选项模块。 */
export const PREFERENCES_MODULE: ShellModule = {
  name: 'preferences',
  pages: [{ id: PAGE_ID, title: TEXT.title, icon: iconSettings, Component: PreferencesPage }],
  commands: [{ id: PREFERENCES_COMMANDS.open, title: TEXT.title, defaultShortcut: 'Ctrl+,', run: (shell) => shell.openPage(PAGE_ID) }],
  menuItems: [{ menu: 'settings', command: PREFERENCES_COMMANDS.open }]
};
