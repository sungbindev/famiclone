// ------------------------------------------------------------------------
// 名称：cpp-properties.ts
// 说明：C/C++ 扩展的 c_cpp_properties.json 与 clangd 的 compile_flags.txt：生成内容、
//       合并已有配置、取出各个配置的名字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：只写或更新名为 fcdev 的配置，保留其他配置；compile_flags.txt 的第一行是标记
//       -DFCDEV_GENERATED，没有标记的同名文件视为用户自己的，不覆盖。
//       解析失败或格式不符时返回空数组。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';
import { formatDefines, type ProjectDescription } from '../project/project-info';

/** fcdev 管理的配置名。 */
export const FCDEV_CONFIGURATION_NAME = 'fcdev';

/** compile_flags.txt 里标记“由扩展生成”的第一行。 */
export const GENERATED_MARKER = '-DFCDEV_GENERATED';

/**
 * 把 fcdev 配置合并进 c_cpp_properties.json：已有 fcdev 配置就替换，没有就追加，其他配置保留。
 * @param existing 已有文件的文本；文件不存在时为 undefined。
 * @param file 文件路径，用在出错提示里。
 * @param description 项目描述。
 * @returns 新文件的文本。
 * @throws FcdevError 已有文件不是合法的配置。
 */
export function mergeCppProperties(existing: string | undefined, file: string, description: ProjectDescription): string {
  let document: { configurations?: unknown } & Record<string, unknown>;
  if (existing === undefined) {
    document = { version: 4, configurations: [] };
  } else {
    try {
      document = JSON.parse(existing) as typeof document;
    } catch (error) {
      throw new FcdevError(`${file} 不是合法的 JSON`, { code: 'SYNC_FILE_INVALID', cause: error });
    }
    if (typeof document !== 'object' || document === null || !Array.isArray(document.configurations)) {
      throw new FcdevError(`${file} 缺少 configurations 数组`, { code: 'SYNC_FILE_INVALID' });
    }
  }
  const configuration = {
    name: FCDEV_CONFIGURATION_NAME,
    includePath: description.includeDirs,
    defines: formatDefines(description.defines),
    cStandard: 'c89',
    intelliSenseMode: 'clang-x86'
  };
  const configurations = document.configurations as Array<{ name?: unknown }>;
  const index = configurations.findIndex((item) => item.name === FCDEV_CONFIGURATION_NAME);
  if (index >= 0) configurations[index] = configuration;
  else configurations.push(configuration);
  return `${JSON.stringify(document, null, 2)}\n`;
}

/**
 * 生成 compile_flags.txt 的内容。
 * @param description 项目描述。
 * @returns 文件文本。
 */
export function buildCompileFlags(description: ProjectDescription): string {
  const lines = [GENERATED_MARKER, ...description.includeDirs.map((dir) => `-I${dir}`), ...formatDefines(description.defines).map((item) => `-D${item}`)];
  return `${lines.join('\n')}\n`;
}

/**
 * 已有的 compile_flags.txt 是否由扩展生成（第一行是标记），只有这样的文件才能覆盖。
 * @param text 已有文件的文本。
 * @returns 由扩展生成时为 true。
 */
export function isGeneratedCompileFlags(text: string): boolean {
  return text.split(/\r?\n/)[0] === GENERATED_MARKER;
}

/**
 * 取出配置文件里每个配置的名字。
 * @param text c_cpp_properties.json 的文本。
 * @returns 配置名，按文件里的顺序；解析失败或没有 configurations 数组时为空数组。
 */
export function parseConfigurationNames(text: string): string[] {
  let document: unknown;
  try {
    document = JSON.parse(text);
  } catch {
    return [];
  }
  const configurations = (document as { configurations?: unknown } | null)?.configurations;
  if (!Array.isArray(configurations)) return [];
  return configurations.flatMap((item: unknown) => {
    const name = (item as { name?: unknown } | null)?.name;
    return typeof name === 'string' ? [name] : [];
  });
}
