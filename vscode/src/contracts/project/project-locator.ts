// ------------------------------------------------------------------------
// 名称：project-locator.ts
// 说明：查找工作区里 FC Dev 项目的接口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：由实现层提供；逻辑层只经这个接口得知工作区里有哪些项目。
// ------------------------------------------------------------------------

/** 查找工作区里的 FC Dev 项目。 */
export interface FcdevProjectLocator {
  /**
   * 列出工作区里全部 FC Dev 项目的根目录。
   * @returns 根目录的绝对路径，不含 node_modules 里的项目。
   */
  findProjectRoots(): Promise<readonly string[]>;
}
