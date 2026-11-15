/*
 * ------------------------------------------------------------------------
 * 名称：input.ts
 * 说明：输入模块在渲染进程中的登记项：“手柄设置”对话框、命令与“设置”菜单项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：登记的内容在 ui/input/input-module.tsx。
 * ------------------------------------------------------------------------
 */

import { INPUT_MODULE } from '../../../ui/input/input-module';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** 输入模块。 */
export const INPUT_RENDERER_MODULE: ShellModule = INPUT_MODULE;
