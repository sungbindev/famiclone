// ------------------------------------------------------------------------
// 名称：params.mjs
// 说明：操作参数的读取与检查：字符串、整数、地址、布尔、数组、枚举，错误统一成 DISASM_PARAM_INVALID。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：地址可以写成整数，也可以写成 "$C000"、"0xC000"、"C000" 这样的十六进制文字；入口等参数里的地址同样处理。
// ------------------------------------------------------------------------
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 抛出参数错误。
 * @param {string} message 说明
 * @param {string} [hint] 建议
 * @returns {never}
 */
export function invalidParam(message, hint) {
  throw new FcdevError(message, { code: 'DISASM_PARAM_INVALID', hint });
}

/**
 * 读取必填字符串。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @returns {string} 值
 */
export function requireString(params, name) {
  const value = params[name];
  if (typeof value !== 'string' || value === '') invalidParam(`参数 ${name} 必须是非空字符串`);
  return value;
}

/**
 * 读取可选字符串。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @returns {string|undefined} 值
 */
export function optionalString(params, name) {
  const value = params[name];
  if (value === undefined) return undefined;
  if (typeof value !== 'string') invalidParam(`参数 ${name} 必须是字符串`);
  return value;
}

/**
 * 读取可选整数。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @param {{min?: number, max?: number}} [range] 取值范围
 * @returns {number|undefined} 值
 */
export function optionalInteger(params, name, { min = 0, max = Number.MAX_SAFE_INTEGER } = {}) {
  const value = params[name];
  if (value === undefined) return undefined;
  if (!Number.isInteger(value) || value < min || value > max) invalidParam(`参数 ${name} 必须是 ${min} 到 ${max} 的整数`);
  return value;
}

/**
 * 读取可选布尔值。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @param {boolean} fallback 缺省值
 * @returns {boolean} 值
 */
export function optionalBoolean(params, name, fallback) {
  const value = params[name];
  if (value === undefined) return fallback;
  if (typeof value !== 'boolean') invalidParam(`参数 ${name} 必须是布尔值`);
  return value;
}

/**
 * 读取可选的枚举值。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @param {string[]} allowed 允许的取值
 * @param {string} [fallback] 缺省值
 * @returns {string|undefined} 值
 */
export function optionalEnum(params, name, allowed, fallback) {
  const value = params[name];
  if (value === undefined) return fallback;
  if (typeof value !== 'string' || !allowed.includes(value)) invalidParam(`参数 ${name} 必须是 ${allowed.join('、')} 之一`);
  return value;
}

/**
 * 把地址参数换成 0 到 0xFFFF 的整数。
 * @param {unknown} value 整数，或 "$C000"、"0xC000"、"C000" 形式的文字
 * @param {string} name 参数名，用于出错提示
 * @returns {number} 地址
 */
export function toAddress(value, name) {
  let number = Number.NaN;
  if (typeof value === 'number') number = value;
  else if (typeof value === 'string') {
    const match = /^(?:\$|0[xX])?([0-9A-Fa-f]{1,4})$/.exec(value.trim());
    if (match !== null) number = Number.parseInt(match[1], 16);
  }
  if (!Number.isInteger(number) || number < 0 || number > 0xffff) invalidParam(`${name} 必须是 16 位地址（整数或十六进制文字，如 "$C000"）`);
  return number;
}

/**
 * 读取可选地址。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @returns {number|undefined} 地址
 */
export function optionalAddress(params, name) {
  return params[name] === undefined ? undefined : toAddress(params[name], name);
}

/**
 * 读取可选数组。
 * @param {Record<string, unknown>} params 参数
 * @param {string} name 参数名
 * @returns {unknown[]|undefined} 数组
 */
export function optionalArray(params, name) {
  const value = params[name];
  if (value === undefined) return undefined;
  if (!Array.isArray(value)) invalidParam(`参数 ${name} 必须是数组`);
  return value;
}

/**
 * 读取工程目录并转成绝对路径。
 * @param {Record<string, unknown>} params 参数
 * @returns {string} 绝对路径
 */
export function projectDirectory(params) {
  return path.resolve(requireString(params, 'projectDir'));
}

/**
 * 编辑项里的地址字段可以写成文字：把 edits 里 address、length 等字段换成整数。
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
