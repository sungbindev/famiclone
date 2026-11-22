// ------------------------------------------------------------------------
// 名称：project-config-reader.ts
// 说明：项目配置 fcdev.json 的读取接口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：数据类型在 shared 里定义，这里重新导出。
// ------------------------------------------------------------------------

import type { ProjectConfig } from '../../shared/project/project-config';

export type { ProjectConfig } from '../../shared/project/project-config';

/** 读取项目配置的能力。 */
export interface ProjectConfigReader {
  /**
   * 从起始目录向上查找最近的 fcdev.json 并读取。
   * @param startDir 起始目录。
   * @param fallbackRoot 找不到配置文件时用作项目根的目录。
   * @returns 项目配置；没有配置文件时返回只含默认值的配置。
   * @throws FcdevError 配置有误。
   */
  read(startDir: string, fallbackRoot: string): ProjectConfig;
}
