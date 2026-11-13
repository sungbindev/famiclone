/*
 * ------------------------------------------------------------------------
 * 名称：output.ts
 * 说明：输出模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { OUTPUT_SCHEMA } from '../../../contracts/output/generated/output';
import type { MainModule } from '../main-module';

/** 输出模块。 */
export const OUTPUT_MAIN_MODULE: MainModule = { schema: OUTPUT_SCHEMA };
