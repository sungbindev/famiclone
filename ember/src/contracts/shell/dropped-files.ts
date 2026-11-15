/*
 * ------------------------------------------------------------------------
 * 名称：dropped-files.ts
 * 说明：拖入窗口的文件的服务契约：登记拖入的文件路径并给出显示名与编号，按编号取回路径。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：路径只在主进程里使用；对渲染进程只给显示名与编号，编号只在本次运行内有效；
 *       登记的路径由预加载脚本从拖入的文件对象取得，渲染进程不能自己指定路径。
 * ------------------------------------------------------------------------
 */

/** 拖入的一个文件：给界面看的部分。 */
export interface DroppedFile {
  /** 编号：本次运行内有效，从 1 开始。 */
  readonly id: number;
  /** 显示名：文件名。 */
  readonly name: string;
}

/** 拖入窗口的文件的服务。 */
export interface DroppedFilesService {
  /**
   * 登记拖入的文件。
   * @param paths 文件路径。
   * @returns 与路径一一对应的编号与显示名。
   */
  register(paths: readonly string[]): readonly DroppedFile[];
  /** 按编号取文件路径；编号没有登记过或已被新的拖入顶掉时为 undefined。 */
  resolve(id: number): string | undefined;
}
