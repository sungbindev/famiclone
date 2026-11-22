// ------------------------------------------------------------------------
// 名称：json-text.ts
// 说明：解析 JSON 文本，并判断解析结果是不是普通对象。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：解析失败抛出带文件名的 FcdevError，调用者不必再包装。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';

/**
 * 判断值是不是普通对象（不是 null，也不是数组）。
 * @param value 任意值。
 * @returns 是普通对象时为 true。
 */
export function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

/**
 * 解析 JSON 文本。
 * @param text JSON 文本。
 * @param file 文件路径，用在出错提示里。
 * @returns 解析后的值。
 * @throws FcdevError 文本不是合法的 JSON。
 */
export function parseJsonText(text: string, file: string): unknown {
  try {
    return JSON.parse(text) as unknown;
  } catch (error) {
    throw new FcdevError(`${file} 不是合法的 JSON`, { code: 'JSON_INVALID', cause: error });
  }
}
