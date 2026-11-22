// ------------------------------------------------------------------------
// 名称：source-charset.ts
// 说明：源文件转码：把 UTF-8 的 C 与汇编源文件转成目标机器用的 GB2312。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：GB2312 的第二个字节不小于 0xA1，不会和 ASCII 的反斜杠、引号冲突，
//       注释与字符串里的汉字都能整体转换；文件不是合法的 UTF-8 时视为已经是目标编码，
//       原样使用；GB2312 之外的字符报错并指出文件、行、列；
//       编码表用 Node 自带的 gb18030 解码器反推，不引入依赖。
// ------------------------------------------------------------------------

import { FcdevError } from '../errors/fcdev-error';

/** GB2312 的区：1-9 区是符号，16-87 区是汉字，对应第一个字节。 */
const GB2312_LEAD_BYTES: ReadonlyArray<readonly [number, number]> = [
  [0xa1, 0xa9],
  [0xb0, 0xf7]
];

/** 反推出的编码表：Unicode 码点到内码（高 8 位是第一个字节）；第一次用到时才建立。 */
let gb2312Table: Map<number, number> | null = null;

/**
 * 取得 GB2312 编码表。
 * @returns 码点到内码的表。
 * @throws FcdevError 当前的 Node 没有 GB 系列解码器。
 */
function getGb2312Table(): Map<number, number> {
  if (gb2312Table !== null) return gb2312Table;
  let decoder: InstanceType<typeof TextDecoder>;
  try {
    decoder = new TextDecoder('gb18030');
  } catch (error) {
    throw new FcdevError('当前的 Node 不支持 GB2312 解码，无法转换源文件编码', {
      code: 'CHARSET_UNSUPPORTED',
      hint: '使用带完整国际化数据的 Node 22 或更高版本，或把源文件另存为 GB2312',
      cause: error
    });
  }
  const table = new Map<number, number>();
  for (const [first, last] of GB2312_LEAD_BYTES) {
    for (let lead = first; lead <= last; lead += 1) {
      for (let trail = 0xa1; trail <= 0xfe; trail += 1) {
        if (lead === 0xd7 && trail >= 0xfa) continue; // 87 区只到 0xD7F9，后面是 GBK 扩展
        const text = decoder.decode(Uint8Array.of(lead, trail));
        const [char, ...rest] = text;
        const code = char?.codePointAt(0);
        if (code !== undefined && rest.length === 0 && char !== '\uFFFD') table.set(code, (lead << 8) | trail);
      }
    }
  }
  gb2312Table = table;
  return table;
}

/**
 * 把 UTF-8 文本转成 GB2312 字节；不是合法 UTF-8 的内容原样返回。
 * @param content 源文件内容。
 * @param label 出错提示里用的文件名。
 * @returns 转换后的内容。
 * @throws FcdevError 有字符不在 GB2312 里。
 */
export function convertToGb2312(content: Uint8Array, label: string): Uint8Array {
  let text: string;
  try {
    text = new TextDecoder('utf-8', { fatal: true }).decode(content);
  } catch {
    return content;
  }
  const bytes: number[] = [];
  let table: Map<number, number> | null = null;
  let line = 1;
  let column = 0;
  for (const char of text) {
    column += char.length;
    const code = char.codePointAt(0) ?? 0;
    if (code < 0x80) {
      bytes.push(code);
      if (code === 0x0a) {
        line += 1;
        column = 0;
      }
      continue;
    }
    table ??= getGb2312Table();
    const mapped = table.get(code);
    if (mapped === undefined) {
      const unicode = `U+${code.toString(16).toUpperCase().padStart(4, '0')}`;
      throw new FcdevError(`${label}:${line}:${column - char.length + 1}：字符“${char}”（${unicode}）不在 GB2312 里，学习机的字库显示不了`, {
        code: 'SOURCE_CHAR_UNSUPPORTED',
        hint: '改用 GB2312 里有的字，或用 \\x 转义写字节'
      });
    }
    bytes.push(mapped >> 8, mapped & 0xff);
  }
  return Uint8Array.from(bytes);
}
