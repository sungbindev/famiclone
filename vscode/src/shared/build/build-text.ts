// ------------------------------------------------------------------------
// 名称：build-text.ts
// 说明：构建用到的文本处理：判断源文件语言、解析依赖文件、拼工具命令行参数、展开多页链接配置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：只拼参数与处理文字，不运行工具；一律带 -g 输出调试信息，依赖文件用 --create-dep 生成。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import type { BankedSpec } from '../targets/target';

/** 源文件的语言。 */
export type SourceLanguage = 'c' | 'asm';

/** 编译与汇编共用的设置。 */
export interface CompileSettings {
  /** 目标名。 */
  readonly cc65Target: string;
  /** CPU 型号。 */
  readonly cpu: string;
  /** 是否优化。 */
  readonly optimize: boolean;
  /** C 头文件目录。 */
  readonly includeDirs: readonly string[];
  /** 汇编引入目录。 */
  readonly asmIncludeDirs: readonly string[];
  /** 预定义宏。 */
  readonly defines: Readonly<Record<string, string | number>>;
}

/** 一次编译或汇编涉及的文件。 */
export interface ToolFiles {
  /** 源文件。 */
  readonly source: string;
  /** 输出文件。 */
  readonly output: string;
  /** 依赖文件。 */
  readonly dependencyFile: string;
}

/** 链接用的参数。 */
export interface LinkerInput {
  /** 链接配置文件。 */
  readonly linkConfig: string;
  /** 输出的二进制文件。 */
  readonly binaryFile: string;
  /** 输出的映射文件。 */
  readonly mapFile: string;
  /** 输出的调试信息文件。 */
  readonly debugFile: string;
  /** 目标文件。 */
  readonly objects: readonly string[];
  /** 库文件。 */
  readonly libraries: readonly string[];
  /** 强制引入的符号。 */
  readonly forceImports?: readonly string[];
}

/** 模板里内存区的展开位置。 */
const MEMORY_MARKER = '%BANK_MEMORY%';

/** 模板里段的展开位置。 */
const SEGMENT_MARKER = '%BANK_SEGMENTS%';

/**
 * 判断源文件的语言。
 * @param file 文件路径。
 * @returns c、asm，或不是源文件时为 null。
 */
export function getSourceLanguage(file: string): SourceLanguage | null {
  const extension = path.extname(file).toLowerCase();
  if (extension === '.c') return 'c';
  if (extension === '.s' || extension === '.asm') return 'asm';
  return null;
}

/**
 * 取出依赖文件里第一条规则的依赖项。
 * @param text 依赖文件内容（make 格式，路径里的空格用反斜杠转义）。
 * @returns 依赖文件路径；没有规则时为空数组。
 */
export function parseDependencyFile(text: string): string[] {
  const joined = text.replace(/\\\r?\n/g, ' ');
  const firstRule = joined.split(/\r?\n/).find((line) => line.trim() !== '');
  if (firstRule === undefined) return [];
  // 规则的冒号后面一定是空白；盘符的冒号后面是路径分隔符，不会被误判
  const separator = firstRule.search(/:(\s|$)/);
  if (separator < 0) return [];
  const dependencies = firstRule.slice(separator + 1).match(/(?:\\ |\S)+/g) ?? [];
  return dependencies.map((item) => item.replaceAll('\\ ', ' '));
}

/**
 * 预定义宏转成 -D 参数。
 * @param defines 宏表。
 * @returns 参数列表。
 */
function defineArguments(defines: Readonly<Record<string, string | number>>): string[] {
  return Object.entries(defines).flatMap(([name, value]) => ['-D', `${name}=${value}`]);
}

/**
 * cc65 编译一个 C 文件的参数。
 * @param settings 编译设置。
 * @param files 源文件、输出的汇编文件与依赖文件。
 * @returns 参数列表。
 */
export function buildCompilerArguments(settings: CompileSettings, files: ToolFiles): string[] {
  return [
    '-t', settings.cc65Target,
    '--cpu', settings.cpu,
    '-g',
    ...(settings.optimize ? ['-Oirs'] : []),
    ...settings.includeDirs.flatMap((dir) => ['-I', dir]),
    ...defineArguments(settings.defines),
    '--create-dep', files.dependencyFile,
    '-o', files.output,
    files.source
  ];
}

/**
 * ca65 汇编一个文件的参数。
 * @param settings 编译设置。
 * @param files 源文件、输出的目标文件与依赖文件。
 * @returns 参数列表。
 */
export function buildAssemblerArguments(settings: CompileSettings, files: ToolFiles): string[] {
  return [
    '-t', settings.cc65Target,
    '--cpu', settings.cpu,
    '-g',
    ...settings.asmIncludeDirs.flatMap((dir) => ['-I', dir, '--bin-include-dir', dir]),
    ...defineArguments(settings.defines),
    '--create-dep', files.dependencyFile,
    '-o', files.output,
    files.source
  ];
}

/**
 * ld65 链接的参数。
 * @param input 链接配置、输出文件、目标文件、库与强制引入的符号。
 * @returns 参数列表。
 */
export function buildLinkerArguments(input: LinkerInput): string[] {
  return [
    '-C', input.linkConfig,
    '-m', input.mapFile,
    '--dbgfile', input.debugFile,
    '-o', input.binaryFile,
    ...(input.forceImports ?? []).flatMap((symbol) => ['-u', symbol]),
    ...input.objects,
    ...input.libraries
  ];
}

/**
 * 把链接配置模板展开成完整的链接配置。
 * @param template 链接配置模板。
 * @param banked 目标的多页定义。
 * @param count 库页个数。
 * @returns 链接配置。
 * @throws FcdevError 模板里缺少展开位置。
 */
export function expandBankedLinkConfig(template: string, banked: BankedSpec, count: number): string {
  const lines = template.split(/\r?\n/);
  const generate = (pattern: string): string[] => {
    const generated: string[] = [];
    for (let bank = banked.generatedFrom; bank <= count; bank += 1) generated.push(`    ${pattern.replaceAll('{n}', String(bank))}`);
    return generated;
  };
  const expand = (marker: string, pattern: string): void => {
    const index = lines.findIndex((line) => line.trim() === marker);
    if (index < 0) throw new FcdevError(`链接配置模板里缺少 ${marker} 一行`, { code: 'TARGET_INVALID' });
    lines.splice(index, 1, ...generate(pattern));
  };
  expand(MEMORY_MARKER, banked.memory);
  expand(SEGMENT_MARKER, banked.segment);
  return lines.join('\n');
}
