/*
 * ------------------------------------------------------------------------
 * 名称：debug.ts
 * 说明：调试服务模块在渲染进程中的登记项：状态栏右侧的服务状态项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：登记的内容在 ui/debug/debug-module.tsx；控制器在这里创建，保证整个界面只有一个。
 * ------------------------------------------------------------------------
 */

import { DebugController } from '../../../ui/debug/debug-controller';
import { createDebugModule } from '../../../ui/debug/debug-module';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** 调试服务模块。 */
export const DEBUG_RENDERER_MODULE: ShellModule = createDebugModule(new DebugController());
