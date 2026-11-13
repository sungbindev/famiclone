/*
 * ------------------------------------------------------------------------
 * 名称：file-dialog.ts
 * 说明：文件选择窗口的抽象：主进程经它请用户选一个文件；实现用系统的文件选择窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只来自用户在系统窗口里的选择，渲染进程不能指定路径。
 * ------------------------------------------------------------------------
 */

/** 文件类型过滤。 */
export interface FileDialogFilter {
  /** 显示名称。 */
  readonly name: string;
  /** 扩展名，不带点，如 nes。 */
  readonly extensions: readonly string[];
}

/** 选择文件的参数。 */
export interface FileDialogOptions {
  /** 窗口标题。 */
  readonly title: string;
  /** 文件类型过滤。 */
  readonly filters: readonly FileDialogFilter[];
}

/** 文件选择窗口。 */
export interface FileDialog {
  /** 请用户选一个文件；用户取消时返回 undefined。 */
  pickFile(options: FileDialogOptions): Promise<string | undefined>;
}
