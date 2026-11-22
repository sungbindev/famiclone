// ------------------------------------------------------------------------
// 名称：param-checks.ts
// 说明：操作参数的读取与检查：字符串、整数、地址、布尔、数组、枚举；错误统一成带指定错误码的
//       FcdevError。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：反汇编与模拟器的操作共用，各自用 createParamChecks 传入自己的错误码；地址可以写成整数，
//       也可以写成 "$C000"、"0xC000"、"C000" 这样的十六进制文字。
// ------------------------------------------------------------------------
import { FcdevError } from '../errors/fcdev-error';

/** 一组参数检查函数，错误码由 createParamChecks 指定。 */
export interface ParamChecks {
  /** 抛出参数错误。 */
  invalidParam(message: string, hint?: string): never;
  /** 读取必填字符串。 */
  requireString(params: Record<string, unknown>, name: string): string;
  /** 读取可选字符串。 */
  optionalString(params: Record<string, unknown>, name: string): string | undefined;
  /** 读取可选整数。 */
  optionalInteger(params: Record<string, unknown>, name: string, range?: { min?: number; max?: number }): number | undefined;
  /** 读取可选布尔值。 */
  optionalBoolean(params: Record<string, unknown>, name: string, fallback: boolean): boolean;
  /** 读取可选的枚举值。 */
  optionalEnum(params: Record<string, unknown>, name: string, allowed: readonly string[], fallback?: string): string | undefined;
  /** 把地址参数换成 0 到 0xFFFF 的整数。 */
  toAddress(value: unknown, name: string): number;
  /** 读取可选地址。 */
  optionalAddress(params: Record<string, unknown>, name: string): number | undefined;
  /** 读取可选数组。 */
  optionalArray(params: Record<string, unknown>, name: string): unknown[] | undefined;
}

/**
 * 创建一组参数检查函数。
 * @param code 参数不合法时的错误码
 * @returns 检查函数
 */
export function createParamChecks(code: string): ParamChecks {
  /**
   * 抛出参数错误。
   * @param message 说明
   * @param [hint] 建议
   * @returns
   */
  function invalidParam(message: string, hint?: string): never {
    throw new FcdevError(message, { code, hint });
  }

  /**
   * 读取必填字符串。
   * @param params 参数
   * @param name 参数名
   * @returns 值
   */
  function requireString(params: Record<string, unknown>, name: string): string {
    const value = params[name];
    if (typeof value !== 'string' || value === '') invalidParam(`参数 ${name} 必须是非空字符串`);
    return value;
  }

  /**
   * 读取可选字符串。
   * @param params 参数
   * @param name 参数名
   * @returns 值
   */
  function optionalString(params: Record<string, unknown>, name: string): string|undefined {
    const value = params[name];
    if (value === undefined) return undefined;
    if (typeof value !== 'string') invalidParam(`参数 ${name} 必须是字符串`);
    return value;
  }

  /**
   * 读取可选整数。
   * @param params 参数
   * @param name 参数名
   * @param [range] 取值范围
   * @returns 值
   */
  function optionalInteger(params: Record<string, unknown>, name: string, { min = 0, max = Number.MAX_SAFE_INTEGER }: {min?: number, max?: number} = {}): number|undefined {
    const value = params[name];
    if (value === undefined) return undefined;
    if (typeof value !== 'number' || !Number.isInteger(value) || value < min || value > max) invalidParam(`参数 ${name} 必须是 ${min} 到 ${max} 的整数`);
    return value;
  }

  /**
   * 读取可选布尔值。
   * @param params 参数
   * @param name 参数名
   * @param fallback 缺省值
   * @returns 值
   */
  function optionalBoolean(params: Record<string, unknown>, name: string, fallback: boolean): boolean {
    const value = params[name];
    if (value === undefined) return fallback;
    if (typeof value !== 'boolean') invalidParam(`参数 ${name} 必须是布尔值`);
    return value;
  }

  /**
   * 读取可选的枚举值。
   * @param params 参数
   * @param name 参数名
   * @param allowed 允许的取值
   * @param [fallback] 缺省值
   * @returns 值
   */
  function optionalEnum(params: Record<string, unknown>, name: string, allowed: readonly string[], fallback?: string): string|undefined {
    const value = params[name];
    if (value === undefined) return fallback;
    if (typeof value !== 'string' || !allowed.includes(value)) invalidParam(`参数 ${name} 必须是 ${allowed.join('、')} 之一`);
    return value;
  }

  /**
   * 把地址参数换成 0 到 0xFFFF 的整数。
   * @param value 整数，或 "$C000"、"0xC000"、"C000" 形式的文字
   * @param name 参数名，用于出错提示
   * @returns 地址
   */
  function toAddress(value: unknown, name: string): number {
    let number = Number.NaN;
    if (typeof value === 'number') number = value;
    else if (typeof value === 'string') {
      const match = /^(?:\$|0[xX])?([0-9A-Fa-f]{1,4})$/.exec(value.trim());
      if (match !== null) number = Number.parseInt(match[1]!, 16);
    }
    if (!Number.isInteger(number) || number < 0 || number > 0xffff) invalidParam(`${name} 必须是 16 位地址（整数或十六进制文字，如 "$C000"）`);
    return number;
  }

  /**
   * 读取可选地址。
   * @param params 参数
   * @param name 参数名
   * @returns 地址
   */
  function optionalAddress(params: Record<string, unknown>, name: string): number|undefined {
    return params[name] === undefined ? undefined : toAddress(params[name], name);
  }

  /**
   * 读取可选数组。
   * @param params 参数
   * @param name 参数名
   * @returns 数组
   */
  function optionalArray(params: Record<string, unknown>, name: string): unknown[]|undefined {
    const value = params[name];
    if (value === undefined) return undefined;
    if (!Array.isArray(value)) invalidParam(`参数 ${name} 必须是数组`);
    return value;
  }

  return { invalidParam, requireString, optionalString, optionalInteger, optionalBoolean, optionalEnum, toAddress, optionalAddress, optionalArray };
}
