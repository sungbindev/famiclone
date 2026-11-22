// ------------------------------------------------------------------------
// 名称：file-target-repository.ts
// 说明：目标定义的读取：读取扩展目录里 targets/<名>/target.json 并校验。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：新增机型只需新增目标目录，不改代码。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { Target, TargetRepository } from '../../contracts/targets/target-repository';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { parseJsonText } from '../../shared/json/json-text';
import { parseTarget } from '../../shared/targets/target-parser';

/** 目标名允许的字符。 */
const TARGET_NAME = /^[a-z0-9-]+$/;

/** 从扩展自带的目标目录读取目标。 */
export class FileTargetRepository implements TargetRepository {
  /**
   * 创建仓库。
   * @param fileSystem 文件系统。
   * @param resources 扩展资源位置。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly resources: ExtensionResources
  ) {}

  /** @inheritdoc */
  listNames(): string[] {
    return this.fileSystem
      .list(this.resources.targetsDirectory)
      .filter((entry) => entry.isDirectory)
      .map((entry) => entry.name)
      .toSorted();
  }

  /** @inheritdoc */
  load(name: string): Target {
    const dir = path.join(this.resources.targetsDirectory, name);
    if (!TARGET_NAME.test(name) || this.fileSystem.kindOf(dir) !== 'directory') {
      throw new FcdevError(`不支持的目标：${name}`, {
        code: 'TARGET_UNKNOWN',
        hint: `可用的目标：${this.listNames().join('、')}`
      });
    }
    const file = path.join(dir, 'target.json');
    return parseTarget(parseJsonText(this.fileSystem.readText(file), file), dir);
  }
}
