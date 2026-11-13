/*
 * ------------------------------------------------------------------------
 * 名称：system.ts
 * 说明：系统模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import { SYSTEM_SCHEMA } from '../../../contracts/system/generated/system';
import type { MainModule } from '../main-module';

/** 系统模块。 */
export const SYSTEM_MAIN_MODULE: MainModule = { schema: SYSTEM_SCHEMA };
