// ------------------------------------------------------------------------
// 名称：tool-paths.ts
// 说明：检查并换算工具输入里的路径：相对路径按第一个工作区文件夹解析，工程目录与构建目录
//       必须在工作区里，映像等文件可以在工作区外但要在确认时说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只做路径的规范化与包含判断，不访问磁盘；换算后的路径都是绝对路径。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { ToolInput, ToolSpec } from '../../contracts/disassembly/tool-spec';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 换算后的输入。 */
export interface ResolvedToolInput {
  /** 路径参数已换成绝对路径的输入。 */
  readonly params: ToolInput;
  /** 工作区之外的路径（只有 outsideAllowed 里的参数才可能出现）。 */
  readonly outside: readonly string[];
}

/** 值是文件路径而不是内置名字：带路径分隔符，或以 .json 结尾。 */
function looksLikePath(value: string): boolean {
  return /[\\/]/.test(value) || value.toLowerCase().endsWith('.json');
}

/** 路径在目录里面（含目录本身）。 */
export function isInside(root: string, target: string): boolean {
  const relative = path.relative(root, target);
  return relative === '' || (!relative.startsWith('..') && !path.isAbsolute(relative));
}

/**
 * 换算路径参数。
 * @param spec 工具。
 * @param input 模型给的输入。
 * @param roots 工作区文件夹的路径。
 * @returns 换算后的输入。
 * @throws UserFacingError 没有工作区文件夹，或不允许的路径在工作区之外。
 */
export function resolveToolInput(spec: ToolSpec, input: ToolInput, roots: readonly string[]): ResolvedToolInput {
  const base = roots[0];
  if (base === undefined) throw new UserFacingError('没有打开工作区文件夹，反汇编工具只能处理工作区里的工程');
  const params: ToolInput = { ...input };
  const outside: string[] = [];

  const resolveOne = (name: string, value: unknown): string => {
    if (typeof value !== 'string' || value === '') throw new UserFacingError(`参数 ${name} 必须是路径`);
    const absolute = path.isAbsolute(value) ? path.resolve(value) : path.resolve(base, value);
    if (!roots.some((root) => isInside(root, absolute))) {
      if (spec.outsideAllowed?.includes(name) !== true) throw new UserFacingError(`参数 ${name} 必须在工作区里：${absolute}`);
      outside.push(absolute);
    }
    return absolute;
  };

  for (const name of spec.paths ?? []) {
    const value = params[name];
    if (value === undefined) continue;
    params[name] = Array.isArray(value) ? value.map((item) => resolveOne(name, item)) : resolveOne(name, value);
  }
  for (const name of spec.references ?? []) {
    const value = params[name];
    if (typeof value === 'string' && looksLikePath(value)) params[name] = resolveOne(name, value);
  }
  return { params, outside };
}
