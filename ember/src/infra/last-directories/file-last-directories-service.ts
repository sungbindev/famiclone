/*
 * ------------------------------------------------------------------------
 * 名称：file-last-directories-service.ts
 * 说明：记住上次目录服务的实现：启动时从本地文件读出并逐项校验（文件不存在或损坏时得到空记录，不报错弹窗），修改后交给文件存储防抖写盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：文件存储由组装根注入；文件格式 { version: 1, directories: { 用途: 目录 } }；用途不合格或目录为空、过长的项读出时跳过。
 * ------------------------------------------------------------------------
 */

import type { LastDirectoriesService } from '../../contracts/shell/last-directories-service';

/** 本地存储：读出保存的内容、安排写入、立即写盘。 */
export interface LastDirectoriesStorage {
  read(): unknown;
  write(value: unknown): void;
  flush(): void;
}

/** 文件格式的版本。 */
const FILE_VERSION = 1;
/** 用途标识的格式。 */
const PURPOSE_PATTERN = /^[A-Za-z][A-Za-z0-9]{0,63}$/;
/** 目录路径的最大长度。 */
const MAX_DIRECTORY_LENGTH = 1024;
/** 最多记住的用途个数。 */
const MAX_PURPOSES = 64;

/** 从保存的内容里取出合格的项。 */
function parseDirectories(saved: unknown): Map<string, string> {
  const result = new Map<string, string>();
  if (typeof saved !== 'object' || saved === null) return result;
  const { version, directories } = saved as { version?: unknown; directories?: unknown };
  if (version !== FILE_VERSION || typeof directories !== 'object' || directories === null) return result;
  for (const [purpose, directory] of Object.entries(directories)) {
    if (result.size >= MAX_PURPOSES) break;
    if (PURPOSE_PATTERN.test(purpose) && typeof directory === 'string' && directory.length > 0 && directory.length <= MAX_DIRECTORY_LENGTH) result.set(purpose, directory);
  }
  return result;
}

/** 保存在本地文件里的记住上次目录服务。 */
export class FileLastDirectoriesService implements LastDirectoriesService {
  private readonly directories: Map<string, string>;

  constructor(private readonly storage: LastDirectoriesStorage) {
    this.directories = parseDirectories(storage.read());
  }

  get(purpose: string): string | undefined {
    return this.directories.get(purpose);
  }

  set(purpose: string, directory: string): void {
    if (!PURPOSE_PATTERN.test(purpose) || directory.length === 0 || directory.length > MAX_DIRECTORY_LENGTH) return;
    if (this.directories.get(purpose) === directory) return;
    if (!this.directories.has(purpose) && this.directories.size >= MAX_PURPOSES) return;
    this.directories.set(purpose, directory);
    this.storage.write({ version: FILE_VERSION, directories: Object.fromEntries(this.directories) });
  }

  flush(): void {
    this.storage.flush();
  }
}
