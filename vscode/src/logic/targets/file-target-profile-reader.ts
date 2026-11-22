// ------------------------------------------------------------------------
// 名称：file-target-profile-reader.ts
// 说明：目标描述的读取：先找扩展自带的 targets/<名>/profile.json，找不到再按路径读文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：描述文件不是合法 JSON 或内容不合法时，错误里写出文件路径；符号集路径换成绝对路径。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { TargetProfile, TargetProfileReader } from '../../contracts/targets/target-profile-reader';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { normalizeProfile, PROFILE_FILE } from '../../shared/targets/target-profile';

/** 内置描述的名字：小写字母、数字与连字符。 */
const BUILTIN_NAME = /^[a-z0-9][a-z0-9-]*$/;

/** 从扩展自带的目标目录与任意路径读取目标描述。 */
export class FileTargetProfileReader implements TargetProfileReader {
  /**
   * 创建读取器。
   * @param fileSystem 文件系统。
   * @param resources 扩展资源位置。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly resources: ExtensionResources
  ) {}

  /** @inheritdoc */
  read(reference: string, cwd: string): { profile: Readonly<TargetProfile>; file: string } {
    const builtin = BUILTIN_NAME.test(reference) ? path.join(this.resources.targetsDirectory, reference, PROFILE_FILE) : undefined;
    const file = builtin !== undefined && this.fileSystem.kindOf(builtin) === 'file' ? builtin : path.resolve(cwd, reference);
    if (this.fileSystem.kindOf(file) !== 'file') {
      throw new FcdevError(`找不到目标描述 ${reference}`, {
        code: 'PROFILE_NOT_FOUND',
        hint: `内置的目标描述有：${this.listBuiltin().join('、') || '没有'}；也可以给 profile.json 的路径`
      });
    }
    let data: unknown;
    try {
      data = JSON.parse(this.fileSystem.readText(file));
    } catch (error) {
      throw new FcdevError(`目标描述 ${file} 不是合法的 JSON`, { code: 'PROFILE_INVALID', cause: error });
    }
    try {
      return { profile: normalizeProfile(data, path.dirname(file)), file };
    } catch (error) {
      if (error instanceof FcdevError && error.code === 'PROFILE_INVALID') {
        throw new FcdevError(`${file}：${error.message}`, { code: 'PROFILE_INVALID', hint: error.hint, cause: error });
      }
      throw error;
    }
  }

  /** @inheritdoc */
  listBuiltin(): string[] {
    const directory = this.resources.targetsDirectory;
    if (this.fileSystem.kindOf(directory) !== 'directory') return [];
    return this.fileSystem
      .list(directory)
      .filter((entry) => entry.isDirectory && this.fileSystem.kindOf(path.join(directory, entry.name, PROFILE_FILE)) === 'file')
      .map((entry) => entry.name)
      .toSorted();
  }
}
