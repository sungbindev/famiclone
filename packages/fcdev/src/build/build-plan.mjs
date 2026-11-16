// ------------------------------------------------------------------------
// 名称：build-plan.mjs
// 说明：把命令行的构建路径解析成构建计划：确定项目配置、目标、源文件、程序名与输出位置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：规则见 packages/fcdev/docs/01-命令行与构建.md 第 2 节：无参数为当前目录；目录整体构成一个程序；文件单独构成一个程序。
// ------------------------------------------------------------------------
import { statSync } from 'node:fs';
import path from 'node:path';
import { loadProjectConfig } from '../config/index.mjs';
import { FcdevError } from '../shared/index.mjs';
import { applyNameRule, loadTarget, resolveTargetOptions } from '../targets/index.mjs';
import { collectSources, getSourceLanguage } from './source-collector.mjs';

/**
 * @typedef {object} BuildPlan
 * @property {'file'|'directory'} mode 构建单位
 * @property {string} sourceBase 源文件的公共根目录，用于生成中间文件名
 * @property {string[]} sources 源文件的绝对路径
 * @property {string} programName 程序名（中间文件目录用）
 * @property {string} outputFile 最终产物的绝对路径
 * @property {string} objectDir 中间文件目录
 * @property {import('../targets/index.mjs').Target} target 目标
 * @property {Record<string, unknown>} targetOptions 合并后的目标专用选项
 * @property {import('../config/index.mjs').ProjectConfig} config 项目配置
 */

/**
 * 读取路径信息，不存在时给出明确的错误。
 * @param {string} target 绝对路径
 * @returns {import('node:fs').Stats} 路径信息
 */
function statOrFail(target) {
  try {
    return statSync(target);
  } catch (error) {
    if (error.code === 'ENOENT') throw new FcdevError(`找不到 ${target}`, { code: 'PATH_NOT_FOUND' });
    throw error;
  }
}

/**
 * 解析构建路径，生成构建计划。
 * @param {{pathArg?: string, cwd: string, targetOverride?: string, targetsDir?: string}} input pathArg 为命令行路径，缺省为当前目录；cwd 为终端当前目录
 * @returns {BuildPlan} 构建计划
 * @throws {FcdevError} 路径不存在、没有指定目标、文件不是源文件等
 */
export function createBuildPlan({ pathArg = '.', cwd, targetOverride, targetsDir }) {
  const absolute = path.resolve(cwd, pathArg);
  const info = statOrFail(absolute);
  const config = loadProjectConfig(info.isDirectory() ? absolute : path.dirname(absolute), cwd);

  const targetName = targetOverride ?? config.target;
  if (!targetName) {
    throw new FcdevError('没有指定目标', { code: 'TARGET_MISSING', hint: '在 fcdev.json 里写 "target"，或用 --target <名> 指定' });
  }
  const target = loadTarget(targetName, targetsDir);
  const targetOptions = resolveTargetOptions(target, config.targetOptions);

  let mode;
  let sourceBase;
  let sources;
  let programName;
  if (info.isDirectory()) {
    mode = 'directory';
    sourceBase = absolute;
    sources = collectSources(absolute, [config.outDir]);
    programName = config.name ?? path.basename(absolute);
  } else {
    if (!getSourceLanguage(absolute)) {
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
    config,
  };
}
