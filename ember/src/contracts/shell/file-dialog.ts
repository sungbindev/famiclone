/*
 * ------------------------------------------------------------------------
 * 名称：file-dialog.ts
 * 说明：文件选择窗口的抽象：主进程经它请用户选一个文件；实现用系统的文件选择窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：路径只来自用户在系统窗口里的选择，渲染进程不能指定路径；每次调用用 purpose 标明用途，相同用途的窗口从上次选定的目录打开（记忆见 last-directories-service.ts）。
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
  /** 用途标识（字母开头的字母与数字），相同用途的窗口记住上次打开的目录。 */
  readonly purpose: string;
}

/** 选择文件夹的参数。 */
export interface DirectoryDialogOptions {
  /** 窗口标题。 */
  readonly title: string;
  /** 用途标识，同上。 */
  readonly purpose: string;
}

/** 文件选择窗口。 */
export interface FileDialog {
  /** 请用户选一个文件；用户取消时返回 undefined。 */
  pickFile(options: FileDialogOptions): Promise<string | undefined>;
  /** 请用户选一个文件夹；用户取消时返回 undefined。 */
  pickDirectory(options: DirectoryDialogOptions): Promise<string | undefined>;
  /** 请用户选一个要保存的文件（文件可以还不存在，已存在时系统窗口会询问是否覆盖）；用户取消时返回 undefined。 */
  pickSaveFile(options: FileDialogOptions): Promise<string | undefined>;
}
