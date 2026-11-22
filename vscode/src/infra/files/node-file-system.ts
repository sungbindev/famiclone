// ------------------------------------------------------------------------
// 名称：node-file-system.ts
// 说明：文件系统接口的 Node 实现：同步读写，读写失败转成带错误码的 FcdevError。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：不依赖 vscode，仓库里的脚本可以直接加载编译后的本文件。
// ------------------------------------------------------------------------

import { mkdirSync, readFileSync, readdirSync, rmSync, statSync, writeFileSync } from 'node:fs';
import type { DirectoryEntry, FileSystem, PathKind } from '../../contracts/files/file-system';
import { FcdevError } from '../../shared/errors/fcdev-error';

/**
 * 判断错误是不是“文件不存在”。
 * @param error 捕获到的错误。
 * @returns 是 ENOENT 或 ENOTDIR 时为 true。
 */
function isMissing(error: unknown): boolean {
  const code = (error as NodeJS.ErrnoException | undefined)?.code;
  return code === 'ENOENT' || code === 'ENOTDIR';
}

/** 经 node:fs 读写文件。 */
export class NodeFileSystem implements FileSystem {
  /** @inheritdoc */
  kindOf(target: string): PathKind | undefined {
    try {
      const info = statSync(target);
      if (info.isDirectory()) return 'directory';
      return info.isFile() ? 'file' : undefined;
    } catch (error) {
      if (isMissing(error)) return undefined;
      throw error;
    }
  }

  /** @inheritdoc */
  list(directory: string): DirectoryEntry[] {
    return readdirSync(directory, { withFileTypes: true }).map((entry) => ({ name: entry.name, isDirectory: entry.isDirectory() }));
  }

  /** @inheritdoc */
  readText(file: string): string {
    try {
      return readFileSync(file, 'utf8');
    } catch (error) {
      throw new FcdevError(`无法读取文件 ${file}`, { code: 'FILE_READ_FAILED', cause: error });
    }
  }

  /** @inheritdoc */
  readBytes(file: string): Uint8Array {
    try {
      return readFileSync(file);
    } catch (error) {
      throw new FcdevError(`无法读取文件 ${file}`, { code: 'FILE_READ_FAILED', cause: error });
    }
  }

  /** @inheritdoc */
  writeText(file: string, text: string): void {
    try {
      writeFileSync(file, text, 'utf8');
    } catch (error) {
      throw new FcdevError(`无法写入文件 ${file}`, { code: 'FILE_WRITE_FAILED', cause: error });
    }
  }

  /** @inheritdoc */
  writeBytes(file: string, data: Uint8Array): void {
    try {
      writeFileSync(file, data);
    } catch (error) {
      throw new FcdevError(`无法写入文件 ${file}`, { code: 'FILE_WRITE_FAILED', cause: error });
    }
  }

  /** @inheritdoc */
  modifiedTime(file: string): number | undefined {
    try {
      return statSync(file).mtimeMs;
    } catch (error) {
      if (isMissing(error)) return undefined;
      throw error;
    }
  }

  /** @inheritdoc */
  ensureDirectory(directory: string): void {
    mkdirSync(directory, { recursive: true });
  }

  /** @inheritdoc */
  removeDirectory(directory: string): void {
    rmSync(directory, { recursive: true, force: true });
  }

  /** @inheritdoc */
  removeFile(file: string): void {
    rmSync(file, { force: true });
  }
}
