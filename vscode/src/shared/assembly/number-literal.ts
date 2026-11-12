// ------------------------------------------------------------------------
// 名称：number-literal.ts
// 说明：汇编数字字面量的解析与显示：识别 $十六进制、0x 十六进制、%二进制和十进制。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只处理单个字面量，不计算表达式。
// ------------------------------------------------------------------------

/**
 * 解析一个数字字面量。
 * @param text 字面量文字，如 `$FFF0`、`0x20`、`%1010`、`42`。
 * @returns 数值；不是单个字面量时为 undefined。
 */
export function parseNumberLiteral(text: string): number | undefined {
  const value = text.trim();
  const hex = /^\$([0-9A-Fa-f]+)$/.exec(value) ?? /^0[xX]([0-9A-Fa-f]+)$/.exec(value);
  if (hex) return Number.parseInt(hex[1] ?? '', 16);
  const binary = /^%([01]+)$/.exec(value);
  if (binary) return Number.parseInt(binary[1] ?? '', 2);
  const decimal = /^(\d+)$/.exec(value);
  if (decimal) return Number.parseInt(decimal[1] ?? '', 10);
  return undefined;
}

/**
 * 生成值的显示文字：单个字面量且不是十进制时，附上十进制数值。
 * @param expression 等号右边的表达式。
 * @returns 显示文字，如 `$FFF0（十进制 65520）`；其他表达式原样返回。
 */
export function describeValue(expression: string): string {
  const text = expression.trim();
  const number = parseNumberLiteral(text);
  if (number === undefined) return text;
  const decimal = String(number);
  return text === decimal ? decimal : `${text}（十进制 ${decimal}）`;
}
