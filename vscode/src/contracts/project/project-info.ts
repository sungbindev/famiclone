// ------------------------------------------------------------------------
// 名称：project-info.ts
// 说明：FC Dev 项目信息的读取接口：按文件找到所属项目并返回目标与头文件目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：数据类型在 shared 里定义，这里重新导出；找不到 FC Dev 项目时提供者返回 undefined。
// ------------------------------------------------------------------------

import type { ProjectInfo } from '../../shared/project/project-info';

export type { ProjectInfo } from '../../shared/project/project-info';

/** 读取项目信息的能力。 */
export interface ProjectInfoProvider {
  /**
   * 取文件所属项目的信息。
   * @param filePath 项目内某个文件的绝对路径。
   * @returns 项目信息；文件不在 FC Dev 项目里，或无法取得时为 undefined。
   */
  get(filePath: string): Promise<ProjectInfo | undefined>;

  /**
   * 丢弃缓存的项目信息，下次读取时重新取。
   * @param root 项目根目录；不给时丢弃全部。
   */
  invalidate(root?: string): void;
}
