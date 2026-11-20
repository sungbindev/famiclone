// ------------------------------------------------------------------------
// 名称：symbol-set.mjs
// 说明：读取符号集文件（ca65 的 .inc）：取出“名字 = 数值”形式的常量，供生成的源码用名字代替硬件寄存器等地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只认行首的“名字 = 数值”（数值是 $ 十六进制、0x 十六进制或十进制，其后可跟 ; 注释），其他行（宏、.import、表达式）忽略；
//       同一个数值有多个名字时，按在文件里出现的先后，第一个用于生成源码。符号集文件本身原样复制进工程，由生成的源码 .include。
// ------------------------------------------------------------------------

/** 一行“名字 = 数值”，数值后面只能跟空白与注释 */
const CONSTANT_LINE = /^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(\$[0-9A-Fa-f]+|0[xX][0-9A-Fa-f]+|[0-9]+)\s*(?:;.*)?$/;

/**
 * 解析符号集文件。
 * @param {string} text 文件内容
 * @returns {Array<{name: string, value: number}>} 常量，按出现的先后
 */
export function parseSymbolSet(text) {
  const constants = [];
  for (const line of text.split(/\r?\n/)) {
    const match = CONSTANT_LINE.exec(line);
    if (match === null) continue;
    const literal = match[2];
    const value = literal.startsWith('$') ? Number.parseInt(literal.slice(1), 16) : Number(literal);
    if (Number.isSafeInteger(value) && value >= 0 && value <= 0xffff) constants.push({ name: match[1], value });
  }
  return constants;
}

/**
 * 把常量按数值索引，同一个数值只留第一个名字。
 * @param {Array<{name: string, value: number}>} constants 常量
 * @returns {Map<number, string>} 数值到名字
 */
export function indexByValue(constants) {
  const names = new Map();
  for (const { name, value } of constants) if (!names.has(value)) names.set(value, name);
  return names;
}
