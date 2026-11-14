/*
 * ------------------------------------------------------------------------
 * 名称：debug.ts
 * 说明：调试服务模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { DEBUG_SCHEMA } from '../../../contracts/debug/generated/debug';
import type { MainModule } from '../main-module';

/** 调试服务模块。 */
export const DEBUG_MAIN_MODULE: MainModule = { schema: DEBUG_SCHEMA };
