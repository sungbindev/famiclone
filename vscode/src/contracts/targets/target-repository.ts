// ------------------------------------------------------------------------
// 名称：target-repository.ts
// 说明：目标定义的读取接口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：数据类型在 shared 里定义，这里重新导出。
// ------------------------------------------------------------------------

import type { Target } from '../../shared/targets/target';

export type { Target, TargetOptionSpec, TargetOptionValue, BankedSpec, RunMode } from '../../shared/targets/target';

/** 读取目标定义的能力。 */
export interface TargetRepository {
  /**
   * 列出全部目标的名称。
   * @returns 目标名，按字母排序。
   */
  listNames(): string[];

  /**
   * 读取一个目标。
   * @param name 目标名。
   * @returns 目标。
   * @throws FcdevError 目标不存在或定义不合法。
   */
  load(name: string): Target;
}
