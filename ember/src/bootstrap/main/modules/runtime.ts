/*
 * ------------------------------------------------------------------------
 * 名称：runtime.ts
 * 说明：运行模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { RUNTIME_SCHEMA } from '../../../contracts/runtime/generated/runtime';
import type { MainModule } from '../main-module';

/** 运行模块。 */
export const RUNTIME_MAIN_MODULE: MainModule = { schema: RUNTIME_SCHEMA };
