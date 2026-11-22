// ------------------------------------------------------------------------
// 名称：build-planner.ts
// 说明：把构建路径解析成构建计划：确定项目配置、目标、源文件、程序名与输出位置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：规则见 vscode/docs 的“构建”一篇：无参数为当前目录；目录整体构成一个程序；
//       文件单独构成一个程序。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ProjectConfigReader } from '../../contracts/project/project-config-reader';
import type { TargetRepository } from '../../contracts/targets/target-repository';
import { FcdevError } from '../../shared/errors/fcdev-error';
import type { BuildInput, BuildPlan } from '../../shared/build/build-plan';
import { getSourceLanguage } from '../../shared/build/build-text';
import { applyNameRule, resolveTargetOptions } from '../../shared/targets/target-settings';

/** 规划构建要用到的能力。 */
export interface PlannerDependencies {
  /** 文件系统。 */
  readonly fileSystem: FileSystem;
  /** 项目配置读取。 */
  readonly configReader: ProjectConfigReader;
  /** 目标定义读取。 */
  readonly targets: TargetRepository;
}

/** 递归收集时跳过的目录名。 */
const SKIPPED_DIRECTORIES = new Set(['build', 'node_modules']);

/**
 * 递归收集目录下的全部源文件。
 * @param fileSystem 文件系统。
 * @param dir 目录的绝对路径。
 * @param excludedDirs 额外跳过的目录（绝对路径，如配置的输出目录）。
 * @returns 源文件的绝对路径，按相对路径排序。
 * @throws FcdevError 目录下没有任何源文件。
 */
export function collectSources(fileSystem: FileSystem, dir: string, excludedDirs: readonly string[] = []): string[] {
  const excluded = new Set(excludedDirs.map((item) => path.resolve(item)));
  const found: string[] = [];
  const walk = (current: string): void => {
    for (const entry of fileSystem.list(current)) {
      const full = path.join(current, entry.name);
      if (entry.isDirectory) {
        if (!SKIPPED_DIRECTORIES.has(entry.name) && !entry.name.startsWith('.') && !excluded.has(full)) walk(full);
      } else if (getSourceLanguage(entry.name) !== null) {
        found.push(full);
      }
    }
  };
  walk(dir);
  found.sort((left, right) => path.relative(dir, left).localeCompare(path.relative(dir, right)));
  if (found.length === 0) {
    throw new FcdevError(`目录 ${dir} 下没有 .c、.s 或 .asm 源文件`, { code: 'NO_SOURCES' });
  }
  return found;
}

/**
 * 解析构建路径，生成构建计划。
 * @param input 构建输入。
 * @param dependencies 规划要用到的能力。
 * @returns 构建计划。
 * @throws FcdevError 路径不存在、没有指定目标、文件不是源文件等。
 */
export function createBuildPlan(input: BuildInput, dependencies: PlannerDependencies): BuildPlan {
  const { fileSystem, configReader, targets } = dependencies;
  const absolute = path.resolve(input.cwd, input.pathArg ?? '.');
  const kind = fileSystem.kindOf(absolute);
  if (kind === undefined) throw new FcdevError(`找不到 ${absolute}`, { code: 'PATH_NOT_FOUND' });
  const config = configReader.read(kind === 'directory' ? absolute : path.dirname(absolute), input.cwd);

  const targetName = input.targetOverride ?? config.target;
  if (targetName === undefined) {
    throw new FcdevError('没有指定目标', { code: 'TARGET_MISSING', hint: '在 fcdev.json 里写 "target"，或在任务里指定 target' });
  }
  const target = targets.load(targetName);
  const targetOptions = resolveTargetOptions(target, config.targetOptions);

  let mode: BuildPlan['mode'];
  let sourceBase: string;
  let sources: string[];
  let programName: string;
  if (kind === 'directory') {
    mode = 'directory';
    sourceBase = absolute;
    sources = collectSources(fileSystem, absolute, [config.outDir]);
    programName = config.name ?? path.basename(absolute);
  } else {
    if (getSourceLanguage(absolute) === null) {
      throw new FcdevError(`${absolute} 不是 .c、.s 或 .asm 源文件`, { code: 'NOT_A_SOURCE' });
    }
    mode = 'file';
    sourceBase = path.dirname(absolute);
    sources = [absolute];
    programName = path.basename(absolute, path.extname(absolute));
  }

  const outputName = applyNameRule(programName, target.nameRule);
  return {
    mode,
    sourceBase,
    sources,
    programName,
    outputFile: path.join(config.outDir, target.name, `${outputName}${target.extension}`),
    objectDir: path.join(config.outDir, 'obj', target.name, programName),
    target,
    targetOptions,
    config
  };
}
