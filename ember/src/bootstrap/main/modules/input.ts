/*
 * ------------------------------------------------------------------------
 * 名称：input.ts
 * 说明：输入模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { INPUT_SCHEMA } from '../../../contracts/input/generated/input';
import type { MainModule } from '../main-module';

/** 输入模块。 */
export const INPUT_MAIN_MODULE: MainModule = { schema: INPUT_SCHEMA };
