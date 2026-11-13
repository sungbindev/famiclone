/*
 * ------------------------------------------------------------------------
 * 名称：module-list.ts
 * 说明：渲染进程的模块清单：外壳从这里读取各模块登记的页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只登记有界面的模块；摘除模块时删除对应的引用与清单项。
 * ------------------------------------------------------------------------
 */

import type { ShellModule } from '../../ui/shell/shell-registry';
import { SYSTEM_RENDERER_MODULE } from './modules/system';

/** 有界面的模块，按登记顺序排列。 */
export const RENDERER_MODULES: readonly ShellModule[] = [SYSTEM_RENDERER_MODULE];
