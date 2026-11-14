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
import { DEBUG_MAIN_MODULE } from './modules/debug';
import { INPUT_MAIN_MODULE } from './modules/input';
import { INSPECTOR_MAIN_MODULE } from './modules/inspector';
import { MACHINES_MAIN_MODULE } from './modules/machines';
import { MAPPERS_MAIN_MODULE } from './modules/mappers';
import { MEDIA_MAIN_MODULE } from './modules/media';
import { OUTPUT_MAIN_MODULE } from './modules/output';
import { PREFERENCES_MAIN_MODULE } from './modules/preferences';
import { RUNTIME_MAIN_MODULE } from './modules/runtime';
import { SYSTEM_MAIN_MODULE } from './modules/system';

/** 全部模块，按登记顺序排列。 */
export const MAIN_MODULES: readonly MainModule[] = [SYSTEM_MAIN_MODULE, PREFERENCES_MAIN_MODULE, MAPPERS_MAIN_MODULE, OUTPUT_MAIN_MODULE, RUNTIME_MAIN_MODULE, INPUT_MAIN_MODULE, INSPECTOR_MAIN_MODULE, CARTRIDGES_MAIN_MODULE, MACHINES_MAIN_MODULE, MEDIA_MAIN_MODULE, DEBUG_MAIN_MODULE];
