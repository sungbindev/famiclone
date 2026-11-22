// ------------------------------------------------------------------------
// 名称：file-project-config-reader.ts
// 说明：项目配置的读取：从起始目录向上查找最近的 fcdev.json，读出并规范化。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：没有配置文件时返回只含默认值的配置，以 fallbackRoot 为项目根。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ProjectConfig, ProjectConfigReader } from '../../contracts/project/project-config-reader';
import { parseJsonText } from '../../shared/json/json-text';
import { CONFIG_FILE_NAME, normalizeConfig } from '../../shared/project/project-config';

/** 经文件系统读取项目配置。 */
export class FileProjectConfigReader implements ProjectConfigReader {
  /**
   * 创建读取器。
   * @param fileSystem 文件系统。
   */
  constructor(private readonly fileSystem: FileSystem) {}

  /** @inheritdoc */
  read(startDir: string, fallbackRoot: string): ProjectConfig {
    const file = this.findConfigFile(startDir);
    if (file === undefined) return normalizeConfig({}, null, fallbackRoot);
    return normalizeConfig(parseJsonText(this.fileSystem.readText(file), file), file, fallbackRoot);
  }

  /**
   * 从起始目录向上查找最近的 fcdev.json。
   * @param startDir 起始目录。
   * @returns 配置文件的绝对路径；找不到时为 undefined。
   */
  private findConfigFile(startDir: string): string | undefined {
    let current = path.resolve(startDir);
    for (;;) {
      const candidate = path.join(current, CONFIG_FILE_NAME);
      if (this.fileSystem.kindOf(candidate) === 'file') return candidate;
      const parent = path.dirname(current);
      if (parent === current) return undefined;
      current = parent;
    }
  }
}
