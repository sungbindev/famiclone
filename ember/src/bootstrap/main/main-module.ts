/*
 * ------------------------------------------------------------------------
 * 名称：main-module.ts
 * 说明：主进程模块登记项的类型。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import type { ModuleSchema } from '../../shared/schema/schema-types';

/** 一个模块在主进程中的登记项。 */
export interface MainModule {
  /** 模块的 Schema：入口层据此接受请求名并校验参数。 */
  readonly schema: ModuleSchema;
}
