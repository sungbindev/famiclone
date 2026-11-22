// ------------------------------------------------------------------------
// 名称：params.ts
// 说明：反汇编操作参数的读取与检查：字符串、整数、地址、布尔、数组、枚举，错误统一成
//       DISASM_PARAM_INVALID，另有工程目录与编辑项的换算。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：通用的检查函数在 shared/param-checks.mjs，这里只指定错误码；地址可以写成整数或 "$C000" 这样
//       的十六进制文字。
// ------------------------------------------------------------------------
import * as path from 'node:path';
import { createParamChecks } from '../../params/param-checks';
import type { EditItem } from '../model/index';

const checks = createParamChecks('DISASM_PARAM_INVALID');

export const { requireString, optionalString, optionalInteger, optionalBoolean, optionalEnum, toAddress, optionalAddress, optionalArray } = checks;

/**
 * 抛出参数错误。
 * @param message 说明
 * @param hint 建议
 * @returns 不返回
 */
export function invalidParam(message: string, hint?: string): never {
  return checks.invalidParam(message, hint);
}

/**
 * 读取工程目录并转成绝对路径。
 * @param params 参数
 * @returns 绝对路径
 */
export function projectDirectory(params: Record<string, unknown>): string {
  return path.resolve(requireString(params, 'projectDir'));
}

/**
 * 编辑项里的地址字段可以写成文字：把 edits 里的 address 换成整数。
 * @param edits 编辑项
 * @returns 换算后的编辑项
 */
export function normalizeEdits(edits: unknown[]): EditItem[] {
  return edits.map((edit, index) => {
    if (edit === null || typeof edit !== 'object' || Array.isArray(edit)) invalidParam(`第 ${index + 1} 个编辑项必须是对象`);
    const result: Record<string, unknown> = { ...edit };
    if (result.address !== undefined) result.address = toAddress(result.address, `第 ${index + 1} 个编辑项的 address`);
    // 编辑项的其余字段由 applyEdits 里各操作逐项检查
    return result as unknown as EditItem;
  });
}
