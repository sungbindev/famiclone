// ------------------------------------------------------------------------
// 名称：project-info.ts
// 说明：FC Dev 项目信息的数据类型，以及项目描述的汇总、转换与格式化。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：项目描述含目标、工具链、头文件目录与预定义宏；预定义宏里带一组编辑器兼容宏：
//       cc65 的头文件大量使用 __fastcall__ 这类关键字，C/C++ 扩展与 clangd 不认识，
//       定义成空宏才不会报语法错误；只用于智能感知，不影响构建。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { ProjectConfig } from './project-config';
import type { Target } from '../targets/target';
import { resolveTargetPath } from '../targets/target-settings';

/** 一个 FC Dev 项目的信息，智能感知用。 */
export interface ProjectInfo {
  /** 项目根目录，即 fcdev.json 所在目录。 */
  readonly root: string;
  /** 目标名，如 nes、sb2000。 */
  readonly targetName: string;
  /** C 头文件目录。 */
  readonly includeDirs: readonly string[];
  /** 汇编引入目录，`.include` 按这些目录查找文件。 */
  readonly asmIncludeDirs: readonly string[];
}

/** 一个 FC Dev 项目的完整描述。 */
export interface ProjectDescription extends ProjectInfo {
  /** 配置文件路径；没有配置文件时为 null。 */
  readonly configFile: string | null;
  /** 目标显示名。 */
  readonly targetTitle: string;
  /** 工具链根目录。 */
  readonly toolchainHome: string;
  /** 预定义宏（含 cc65 自带的目标宏与编辑器兼容宏）。 */
  readonly defines: Readonly<Record<string, string | number>>;
  /** 输出目录。 */
  readonly outDir: string;
}

/** 只供编辑器使用的兼容宏：cc65 的调用约定关键字定义成空。 */
const EDITOR_COMPATIBILITY_DEFINES: Readonly<Record<string, string | number>> = { __fastcall__: '', __cdecl__: '' };

/**
 * 汇总项目描述。
 * @param config 项目配置。
 * @param target 目标。
 * @param toolchainHome 工具链根目录。
 * @returns 项目描述。
 */
export function describeProject(config: ProjectConfig, target: Target, toolchainHome: string): ProjectDescription {
  const resolveAll = (values: readonly string[]): string[] => values.map((value) => resolveTargetPath(target, value, toolchainHome));
  return {
    root: config.root,
    configFile: config.file,
    targetName: target.name,
    targetTitle: target.title,
    toolchainHome,
    includeDirs: [...resolveAll(target.include), ...config.include, path.join(toolchainHome, 'include')],
    asmIncludeDirs: [...resolveAll(target.asminclude), ...config.include, path.join(toolchainHome, 'asminc')],
    defines: { ...EDITOR_COMPATIBILITY_DEFINES, __CC65__: 1, [`__${target.cc65Target.toUpperCase()}__`]: 1, ...target.defines, ...config.define },
    outDir: config.outDir
  };
}

/**
 * 取出智能感知用的项目信息。
 * @param description 项目描述。
 * @returns 项目信息。
 */
export function toProjectInfo(description: ProjectDescription): ProjectInfo {
  return {
    root: description.root,
    targetName: description.targetName,
    includeDirs: description.includeDirs,
    asmIncludeDirs: description.asmIncludeDirs
  };
}

/**
 * 把预定义宏转成 NAME=VALUE 形式。
 * @param defines 宏表。
 * @returns 宏列表。
 */
export function formatDefines(defines: Readonly<Record<string, string | number>>): string[] {
  return Object.entries(defines).map(([name, value]) => `${name}=${value}`);
}

/**
 * 把项目描述格式化成多行文字。
 * @param description 项目描述。
 * @returns 多行文字。
 */
export function formatProjectDescription(description: ProjectDescription): string {
  return [
    `项目根目录：${description.root}`,
    `配置文件：${description.configFile ?? '（无）'}`,
    `目标：${description.targetName}（${description.targetTitle}）`,
    `工具链：${description.toolchainHome}`,
    `输出目录：${description.outDir}`,
    `头文件目录：\n  ${description.includeDirs.join('\n  ')}`,
    `汇编引入目录：\n  ${description.asmIncludeDirs.join('\n  ')}`
  ].join('\n');
}
