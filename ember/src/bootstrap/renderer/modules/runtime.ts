/*
 * ------------------------------------------------------------------------
 * 名称：runtime.ts
 * 说明：运行模块在渲染进程中的登记项：登记运行页（画面与开发期操作条）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { RuntimePage } from '../../../ui/runtime/RuntimePage';
import type { ShellModule } from '../../../ui/shell/shell-registry';

/** 运行模块；运行页占满外壳的剩余空间。 */
export const RUNTIME_RENDERER_MODULE: ShellModule = { id: 'runtime', Page: RuntimePage, fill: true };
