// ------------------------------------------------------------------------
// 名称：params.mjs
// 说明：反汇编操作参数的读取与检查：字符串、整数、地址、布尔、数组、枚举，错误统一成 DISASM_PARAM_INVALID，另有工程目录与编辑项的换算。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：通用的检查函数在 shared/param-checks.mjs，这里只指定错误码；地址可以写成整数或 "$C000" 这样的十六进制文字。
// ------------------------------------------------------------------------
import path from 'node:path';
import { createParamChecks } from '../../shared/index.mjs';

const checks = createParamChecks('DISASM_PARAM_INVALID');

export const { invalidParam, requireString, optionalString, optionalInteger, optionalBoolean, optionalEnum, toAddress, optionalAddress, optionalArray } = checks;

/**
 * 读取工程目录并转成绝对路径。
 * @param {Record<string, unknown>} params 参数
 * @returns {string} 绝对路径
 */
export function projectDirectory(params) {
  return path.resolve(requireString(params, 'projectDir'));
}

/**
 * 编辑项里的地址字段可以写成文字：把 edits 里的 address 换成整数。
 * @param {unknown[]} edits 编辑项
 * @returns {Array<Record<string, unknown>>} 换算后的编辑项
 */
export function normalizeEdits(edits) {
  return edits.map((edit, index) => {
    if (edit === null || typeof edit !== 'object' || Array.isArray(edit)) invalidParam(`第 ${index + 1} 个编辑项必须是对象`);
    const result = { ...edit };
    if (result.address !== undefined) result.address = toAddress(result.address, `第 ${index + 1} 个编辑项的 address`);
    return result;
  });
}
