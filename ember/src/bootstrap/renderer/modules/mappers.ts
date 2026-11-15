/*
 * ------------------------------------------------------------------------
 * 名称：mappers.ts
 * 说明：mapper 插件模块在渲染进程中的登记项：“mapper 插件”对话框、命令与“设置”菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：登记的内容在 ui/mappers/mappers-module.tsx。
 * ------------------------------------------------------------------------
 */

import { MAPPERS_MODULE } from '../../../ui/mappers/mappers-module';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** mapper 插件模块。 */
export const MAPPERS_RENDERER_MODULE: ShellModule = MAPPERS_MODULE;
