/*
 * ------------------------------------------------------------------------
 * 名称：system.ts
 * 说明：系统模块在渲染进程中的登记项：登记版本信息页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import type { ShellModule } from '../../../ui/shell/shell-registry';
import { SystemVersionPage } from '../../../ui/system/SystemVersionPage';

/** 系统模块。 */
export const SYSTEM_RENDERER_MODULE: ShellModule = { id: 'system', Page: SystemVersionPage };
