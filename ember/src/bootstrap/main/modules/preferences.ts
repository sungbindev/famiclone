/*
 * ------------------------------------------------------------------------
 * 名称：preferences.ts
 * 说明：首选项模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { PREFERENCES_SCHEMA } from '../../../contracts/preferences/generated/preferences';
import type { MainModule } from '../main-module';

/** 首选项模块。 */
export const PREFERENCES_MAIN_MODULE: MainModule = { schema: PREFERENCES_SCHEMA };
