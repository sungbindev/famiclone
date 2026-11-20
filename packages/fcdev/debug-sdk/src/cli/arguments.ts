/*
 * ------------------------------------------------------------------------
 * 名称：arguments.ts
 * 说明：终端客户端的参数解析：call 命令的请求参数（JSON 对象或 key=value 列表），以及结果里二进制数据的显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

/** 命令行用法错误（退出码 2）。 */
export class UsageError extends Error {
  override name = 'UsageError';
}

/** 十六进制整数的写法：0x8000 或 $8000。 */
const HEX_NUMBER = /^(?:0x|\$)([0-9a-f]+)$/i;

/** 一个值：十六进制整数（0x8000、$8000）是数字；能按 JSON 解析的（数字、true、false、null、字符串、数组、对象）按 JSON，其余当字符串。 */
function parseValue(text: string): unknown {
  const hex = HEX_NUMBER.exec(text);
  if (hex?.[1] !== undefined) return Number.parseInt(hex[1], 16);
  try {
    return JSON.parse(text);
  } catch {
    return text;
  }
}

/**
 * 解析 call 命令的请求参数：没有参数是空对象；一个以 { 开头的参数按 JSON 对象；否则每个参数是 key=value。
 * @throws UsageError 写法不对。
 */
export function parseCallParams(tokens: readonly string[]): Record<string, unknown> {
  if (tokens.length === 0) return {};
  const first = tokens[0] ?? '';
  if (first.startsWith('{')) {
    if (tokens.length > 1) throw new UsageError('参数写成 JSON 对象时只能有一个参数');
    let parsed: unknown;
    try {
      parsed = JSON.parse(first);
    } catch {
      throw new UsageError('参数不是合法的 JSON 对象');
    }
    if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) throw new UsageError('参数必须是 JSON 对象');
    return parsed as Record<string, unknown>;
  }
  const params: Record<string, unknown> = {};
  for (const token of tokens) {
    const separator = token.indexOf('=');
    if (separator <= 0) throw new UsageError(`参数 ${token} 不是 key=value 的写法`);
    params[token.slice(0, separator)] = parseValue(token.slice(separator + 1));
  }
  return params;
}

/** 把结果里的二进制（Uint8Array）换成说明文字，其余原样，供打印。 */
export function printable(value: unknown): unknown {
  if (value instanceof Uint8Array) return `<二进制 ${value.byteLength} 字节>`;
  if (Array.isArray(value)) return value.map(printable);
  if (value !== null && typeof value === 'object') return Object.fromEntries(Object.entries(value).map(([key, child]) => [key, printable(child)]));
  return value;
}
