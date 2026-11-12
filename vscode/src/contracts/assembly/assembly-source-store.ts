// ------------------------------------------------------------------------
// 名称：assembly-source-store.ts
// 说明：汇编源文件的读取能力：读取一个文件的文本，列出工作区里的全部汇编文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：这是逻辑层需要的外部能力接口，由实现层提供；逻辑层不直接访问文件系统。
// ------------------------------------------------------------------------

/** 汇编源文件的存取。 */
export interface AssemblySourceStore {
  /**
   * 读取一个汇编文件的文本。
   * @param path 文件的绝对路径。
   * @returns 文件文本；文件不存在或无法读取时为 undefined。
   */
  read(path: string): Promise<string | undefined>;

  /**
   * 列出工作区里的全部汇编文件（.s、.asm、.inc），不含 node_modules 与构建输出目录。
   * @returns 文件的绝对路径。
   */
  listWorkspaceFiles(): Promise<readonly string[]>;
}
