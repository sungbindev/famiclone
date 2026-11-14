/*
 * ------------------------------------------------------------------------
 * 名称：runtime.ts
 * 说明：运行模块在渲染进程中的登记项：创建运行页的控制器，登记“运行”页（含边栏）、活动栏按钮、命令、菜单项、状态栏项与常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：登记的内容在 ui/runtime/runtime-module.tsx；控制器在这里创建，保证整个界面只有一个。
 * ------------------------------------------------------------------------
 */

import { RuntimeController } from '../../../ui/runtime/runtime-controller';
import { createRuntimeModule } from '../../../ui/runtime/runtime-module';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** 运行模块。 */
export const RUNTIME_RENDERER_MODULE: ShellModule = createRuntimeModule(new RuntimeController());
