// ------------------------------------------------------------------------
// 名称：target-settings.ts
// 说明：目标定义的取值规则：路径值换成绝对路径，合并用户给的专用选项，按规则规范化程序名。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：程序名规则目前只有 DOS 8.3 一条，超长或含非法字符时报错，不静默截断。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import type { Target, TargetOptionValue } from './target';

/** 路径值里表示“相对于工具链根目录”的前缀。 */
const TOOLCHAIN_PREFIX = 'toolchain:';

/** DOS 8.3 的主名：1 到 8 个字母、数字、下划线或连字符。 */
const DOS_BASE_NAME = /^[A-Za-z0-9_-]{1,8}$/;

/**
 * 把目标定义里的路径值换成绝对路径。
 * @param target 目标。
 * @param value 路径值：相对目标目录，或以 toolchain: 开头表示相对工具链根目录。
 * @param toolchainHome 工具链根目录。
 * @returns 绝对路径。
 */
export function resolveTargetPath(target: Target, value: string, toolchainHome: string): string {
  if (value.startsWith(TOOLCHAIN_PREFIX)) return path.join(toolchainHome, value.slice(TOOLCHAIN_PREFIX.length));
  return path.join(target.dir, value);
}

/**
 * 合并用户给的目标专用选项与默认值，并校验。
 * @param target 目标。
 * @param given 配置里的 targetOptions。
 * @returns 完整的选项。
 * @throws FcdevError 出现目标没有定义的选项，或类型不对。
 */
export function resolveTargetOptions(target: Target, given: Readonly<Record<string, unknown>>): Record<string, TargetOptionValue> {
  const result: Record<string, TargetOptionValue> = {};
  for (const [key, spec] of Object.entries(target.options)) result[key] = spec.default;
  for (const [key, value] of Object.entries(given)) {
    const spec = target.options[key];
    if (spec === undefined) {
      const names = Object.keys(target.options);
      throw new FcdevError(`目标 ${target.name} 没有选项 ${key}`, {
        code: 'TARGET_OPTION_UNKNOWN',
        hint: names.length > 0 ? `可用的选项：${names.join('、')}` : '该目标没有可配置的选项'
      });
    }
    if (typeof value !== spec.type) {
      throw new FcdevError(`目标 ${target.name} 的选项 ${key} 类型应为 ${spec.type}`, { code: 'TARGET_OPTION_INVALID' });
    }
    if (target.banked?.option === key && (!Number.isInteger(value) || (value as number) < 0 || (value as number) > target.banked.maxBanks)) {
      throw new FcdevError(`目标 ${target.name} 的选项 ${key} 必须是 0 到 ${target.banked.maxBanks} 的整数`, {
        code: 'TARGET_OPTION_INVALID'
      });
    }
    result[key] = value as TargetOptionValue;
  }
  return result;
}

/**
 * 按目标的程序名规则校验并规范化程序名。
 * @param name 程序名（不含扩展名）。
 * @param rule 目标声明的规则；null 表示不限制。
 * @returns 规范化后的程序名。
 * @throws FcdevError 程序名不符合规则。
 */
export function applyNameRule(name: string, rule: string | null): string {
  if (rule === null) return name;
  if (rule === 'dos-8.3') {
    if (!DOS_BASE_NAME.test(name)) {
      throw new FcdevError(`程序名“${name}”不符合 DOS 8.3 规则：主名为 1 到 8 个字母、数字、下划线或连字符`, {
        code: 'NAME_INVALID',
        hint: '在 fcdev.json 里用 name 指定程序名，或改文件名'
      });
    }
    return name.toUpperCase();
  }
  throw new FcdevError(`未知的程序名规则：${rule}`, { code: 'TARGET_INVALID' });
}
