/*
 * ------------------------------------------------------------------------
 * 名称：module-list.ts
 * 说明：主进程的模块清单：只有登记在这里的模块，其请求才会被入口层接受。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：清单须包含 schema/ 下的全部模块，由 tools/check/check-modules.mjs 检查；摘除模块时删除对应的引用与清单项。
 * ------------------------------------------------------------------------
 */

import type { MainModule } from './main-module';
import { CARTRIDGES_MAIN_MODULE } from './modules/cartridges';
import { MAPPERS_MAIN_MODULE } from './modules/mappers';
import { OUTPUT_MAIN_MODULE } from './modules/output';
import { RUNTIME_MAIN_MODULE } from './modules/runtime';
import { SYSTEM_MAIN_MODULE } from './modules/system';

/** 全部模块，按登记顺序排列。 */
export const MAIN_MODULES: readonly MainModule[] = [SYSTEM_MAIN_MODULE, MAPPERS_MAIN_MODULE, OUTPUT_MAIN_MODULE, RUNTIME_MAIN_MODULE, CARTRIDGES_MAIN_MODULE];
