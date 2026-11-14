/*
 * ------------------------------------------------------------------------
 * 名称：inspector.ts
 * 说明：检查器模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { INSPECTOR_SCHEMA } from '../../../contracts/inspector/generated/inspector';
import type { MainModule } from '../main-module';

/** 检查器模块。 */
export const INSPECTOR_MAIN_MODULE: MainModule = { schema: INSPECTOR_SCHEMA };
