// ------------------------------------------------------------------------
// 名称：file-system.ts
// 说明：文件系统的读写接口：构建、目标与配置的读取都经它进行，便于测试时替换。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：读写失败抛出 FcdevError（带错误码 FILE_READ_FAILED、FILE_WRITE_FAILED）；路径都是绝对路径。
// ------------------------------------------------------------------------

/** 路径的种类。 */
export type PathKind = 'file' | 'directory';

/** 目录里的一项。 */
export interface DirectoryEntry {
  /** 名字，不含目录。 */
  readonly name: string;
  /** 是目录时为 true。 */
  readonly isDirectory: boolean;
}

/** 同步的文件系统能力。 */
export interface FileSystem {
  /**
   * 取路径的种类。
   * @param target 路径。
   * @returns 文件或目录；不存在时为 undefined。
   */
  kindOf(target: string): PathKind | undefined;

  /**
   * 列出目录里的内容。
   * @param directory 目录。
   * @returns 目录里的各项。
   */
  list(directory: string): DirectoryEntry[];

  /**
   * 以 UTF-8 读取文本文件。
   * @param file 文件路径。
   * @returns 文本内容。
   */
  readText(file: string): string;

  /**
   * 读取二进制文件。
   * @param file 文件路径。
   * @returns 文件内容。
   */
  readBytes(file: string): Uint8Array;

  /**
   * 写入文本文件（UTF-8），目录必须已存在。
   * @param file 文件路径。
   * @param text 文本内容。
   */
  writeText(file: string, text: string): void;

  /**
   * 写入二进制文件，目录必须已存在。
   * @param file 文件路径。
   * @param data 文件内容。
   */
  writeBytes(file: string, data: Uint8Array): void;

  /**
   * 取文件的修改时间。
   * @param file 文件路径。
   * @returns 毫秒时间戳；文件不存在时为 undefined。
   */
  modifiedTime(file: string): number | undefined;

  /**
   * 递归创建目录，已存在时不报错。
   * @param directory 目录路径。
   */
  ensureDirectory(directory: string): void;

  /**
   * 递归删除目录及其内容，不存在时不报错。
   * @param directory 目录路径。
   */
  removeDirectory(directory: string): void;

  /**
   * 删除一个文件，不存在时不报错。
   * @param file 文件路径。
   */
  removeFile(file: string): void;
}
