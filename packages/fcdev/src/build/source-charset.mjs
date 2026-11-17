// ------------------------------------------------------------------------
// 名称：source-charset.mjs
// 说明：源文件转码：目标声明了 sourceCharset 时，编译前把 UTF-8 的 C 与汇编源文件转成目标机器用的编码，写成副本交给编译器。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-18
// 备注：目前只有 GB2312（学习机的字库按它取字模）。GB2312 的第二个字节不小于 0xA1，不会和 ASCII 的反斜杠、引号冲突，所以注释与字符串里的汉字都能整体转换；
//       文件不是合法的 UTF-8 时视为已经是目标编码，原样使用；机器字库没有的字符（GB2312 之外）报错，指出文件、行、列；
//       编码表用 Node 自带的 gb18030 解码器反推，不引入依赖；C 副本开头加 #line 指令，让错误信息与调试信息仍指向原文件，汇编靠替换工具输出里的文件名。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';

/** GB2312 的区：1-9 区是符号，16-87 区是汉字，对应第一个字节 */
const GB2312_LEAD_BYTES = [[0xa1, 0xa9], [0xb0, 0xf7]];

/** 反推出的编码表：Unicode 码点到两个字节（高字节在前）；第一次用到时才建立 */
let gb2312Table = null;

/**
 * 取得 GB2312 编码表。
 * @returns {Map<number, number>} 码点到内码（高 8 位是第一个字节）
 * @throws {FcdevError} 当前的 Node 没有 GB 系列解码器
 */
function getGb2312Table() {
  if (gb2312Table !== null) return gb2312Table;
  let decoder;
  try {
    decoder = new TextDecoder('gb18030');
  } catch {
    throw new FcdevError('当前的 Node 不支持 GB2312 解码，无法转换源文件编码', { code: 'CHARSET_UNSUPPORTED', hint: '使用带完整国际化数据的 Node 22 或更高版本，或把源文件另存为 GB2312' });
  }
  const table = new Map();
  for (const [first, last] of GB2312_LEAD_BYTES) {
    for (let lead = first; lead <= last; lead += 1) {
      for (let trail = 0xa1; trail <= 0xfe; trail += 1) {
        if (lead === 0xd7 && trail >= 0xfa) continue; // 87 区只到 0xD7F9，后面是 GBK 扩展
        const text = decoder.decode(Uint8Array.of(lead, trail));
        const [char, ...rest] = text;
        if (char !== undefined && rest.length === 0 && char !== '\uFFFD') table.set(char.codePointAt(0), (lead << 8) | trail);
      }
    }
  }
  gb2312Table = table;
  return table;
}

/**
 * 把 UTF-8 文本转成 GB2312 字节；不是合法 UTF-8 的内容原样返回。
 * @param {Buffer} buffer 源文件内容
 * @param {string} label 出错提示里用的文件名
 * @returns {Buffer} 转换后的内容
 * @throws {FcdevError} 有字符不在 GB2312 里
 */
export function convertToGb2312(buffer, label) {
  let text;
  try {
    text = new TextDecoder('utf-8', { fatal: true }).decode(buffer);
  } catch {
    return buffer;
  }
  const bytes = [];
  let table = null;
  let line = 1;
  let column = 0;
  for (const char of text) {
    column += char.length;
    const code = char.codePointAt(0);
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
        hint: '改用 GB2312 里有的字，或用 \\x 转义写字节',
      });
    }
    bytes.push(mapped >> 8, mapped & 0xff);
  }
  return Buffer.from(bytes);
}

/**
 * 为一个源文件准备转码后的副本。
 * @param {object} input 参数
 * @param {string} input.source 源文件绝对路径
 * @param {string} input.base 中间文件的公共前缀（不含扩展名）
 * @param {'c'|'asm'} input.language 源文件语言
 * @returns {{file: string, restoreText: (text: string) => string}} 副本路径，以及把工具输出里的副本路径换回原文件的函数
 * @throws {FcdevError} 有字符不在 GB2312 里
 */
export function prepareGb2312Source({ source, base, language }) {
  const file = `${base}.gb2312${path.extname(source)}`;
  const converted = convertToGb2312(readFileSync(source), source);
  if (language === 'c') {
    const original = source.replaceAll('\\', '\\\\').replaceAll('"', '\\"');
    writeFileSync(file, Buffer.concat([Buffer.from(`#line 1 "${original}"\n`, 'utf8'), converted]));
    return { file, restoreText: (text) => text };
  }
  writeFileSync(file, converted);
  return { file, restoreText: (text) => text.replaceAll(file, source) };
}
