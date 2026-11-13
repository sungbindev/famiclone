/*
 * ------------------------------------------------------------------------
 * 名称：mappers.ts
 * 说明：mapper 插件模块在主进程中的登记项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { MAPPERS_SCHEMA } from '../../../contracts/mappers/generated/mappers';
import type { MainModule } from '../main-module';

/** mapper 插件模块。 */
export const MAPPERS_MAIN_MODULE: MainModule = { schema: MAPPERS_SCHEMA };
