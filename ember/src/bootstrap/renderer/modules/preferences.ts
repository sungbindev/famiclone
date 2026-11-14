/*
 * ------------------------------------------------------------------------
 * 名称：preferences.ts
 * 说明：首选项模块在渲染进程中的登记项：“首选项”页、命令与“设置”菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：登记的内容在 ui/preferences/preferences-module.tsx。
 * ------------------------------------------------------------------------
 */

import { PREFERENCES_MODULE } from '../../../ui/preferences/preferences-module';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** 首选项模块。 */
export const PREFERENCES_RENDERER_MODULE: ShellModule = PREFERENCES_MODULE;
