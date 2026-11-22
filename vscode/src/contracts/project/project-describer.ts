// ------------------------------------------------------------------------
// 名称：project-describer.ts
// 说明：项目描述的读取接口：汇总项目的目标、工具链与头文件目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：数据类型在 shared 里定义，这里重新导出。
// ------------------------------------------------------------------------

import type { ProjectDescription } from '../../shared/project/project-info';

export type { ProjectDescription } from '../../shared/project/project-info';

/** 汇总项目描述的能力。 */
export interface ProjectDescriber {
  /**
   * 描述 cwd 所在的项目：从 cwd 向上找最近的 fcdev.json。
   * @param input 起始目录与目标覆盖；没有配置文件时必须给目标。
   * @returns 项目描述。
   * @throws FcdevError 没有指定目标，或找不到目标、工具链。
   */
  describe(input: { readonly cwd: string; readonly target?: string | undefined }): ProjectDescription;
}
