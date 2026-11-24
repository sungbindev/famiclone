// ------------------------------------------------------------------------
// 名称：file-cpp-configuration-inspector.ts
// 说明：C/C++ 配置检查器的实现：读取项目 .vscode/c_cpp_properties.json，找出 FC Dev 以外的配置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：文件不存在算没有其他配置；其他读取错误（如没有权限）原样抛出。
// ------------------------------------------------------------------------

import { readFile } from 'node:fs/promises';
import * as path from 'node:path';
import type { CppConfigurationInspector } from '../../contracts/c-intellisense/cpp-configuration-inspector';
import { FCDEV_CONFIGURATION_NAME, parseConfigurationNames } from '../../shared/c-intellisense/cpp-properties';

/** 配置文件相对项目根目录的路径片段。 */
const PROPERTIES_PATH = ['.vscode', 'c_cpp_properties.json'] as const;

/** 经文件系统读取配置文件的检查器。 */
export class FileCppConfigurationInspector implements CppConfigurationInspector {
  /** @inheritdoc */
  async hasOtherConfigurations(root: string): Promise<boolean> {
    let text: string;
    try {
      text = await readFile(path.join(root, ...PROPERTIES_PATH), 'utf8');
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code === 'ENOENT') return false;
      throw error;
    }
    return parseConfigurationNames(text).some((name) => name !== FCDEV_CONFIGURATION_NAME);
  }
}
