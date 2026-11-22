// ------------------------------------------------------------------------
// 名称：extension-resources-factory.ts
// 说明：由扩展目录得到扩展自带资源的位置：版本号、目标定义、工具链、项目模板与使用文档目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：扩展目录就是含 package.json 的目录；仓库里的脚本也用它取得资源位置。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import { FcdevError } from '../../shared/errors/fcdev-error';

/**
 * 由扩展目录得到资源位置，版本号读自 package.json。
 * @param extensionDirectory 扩展目录的绝对路径。
 * @returns 资源位置。
 * @throws FcdevError 读不到 package.json 里的版本号。
 */
export function createExtensionResources(extensionDirectory: string): ExtensionResources {
  const manifest = path.join(extensionDirectory, 'package.json');
  let version: unknown;
  try {
    version = (JSON.parse(readFileSync(manifest, 'utf8')) as { version?: unknown }).version;
  } catch (error) {
    throw new FcdevError(`无法读取扩展清单 ${manifest}`, { code: 'FILE_READ_FAILED', cause: error });
  }
  if (typeof version !== 'string') throw new FcdevError(`${manifest} 里没有版本号`, { code: 'JSON_INVALID' });
  return {
    version,
    extensionDirectory,
    targetsDirectory: path.join(extensionDirectory, 'targets'),
    toolchainDirectory: path.join(extensionDirectory, 'toolchain'),
    templatesDirectory: path.join(extensionDirectory, 'templates'),
    docsDirectory: path.join(extensionDirectory, 'docs')
  };
}
