// ------------------------------------------------------------------------
// 名称：c-api-service.ts
// 说明：目标 C 接口文档的读取接口：由项目或目标名得到全部带文档注释的函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：数据类型与查询、格式化函数在 shared 里；文档来自目标头文件里的文档注释，不另存一份。
// ------------------------------------------------------------------------

import type { ApiFunction } from '../../shared/c-api/api-functions';

export type { ApiFunction } from '../../shared/c-api/api-functions';

/** 一个目标的 C 接口。 */
export interface TargetApi {
  /** 目标名。 */
  readonly targetName: string;
  /** 目标显示名。 */
  readonly targetTitle: string;
  /** 全部带文档注释的函数，按头文件名与声明顺序。 */
  readonly functions: readonly ApiFunction[];
}

/** 读取目标 C 接口的能力。 */
export interface CApiService {
  /**
   * 读取目标的 C 接口。
   * @param input 起始目录与目标覆盖；不给目标时取起始目录向上最近的 fcdev.json 里的目标。
   * @returns 目标的 C 接口。
   * @throws FcdevError 没有指定目标，或目标没有 C 接口文档。
   */
  describe(input: { readonly cwd: string; readonly target?: string | undefined }): TargetApi;
}
