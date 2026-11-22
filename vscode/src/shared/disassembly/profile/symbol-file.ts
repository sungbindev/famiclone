// ------------------------------------------------------------------------
// 名称：symbol-file.ts
// 说明：读取用户提供的符号文件：每行“名字 = 地址”或“名字 地址”，用于给地址起名字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：地址按十六进制，可带 $ 或 0x 前缀；以 ; 或 # 开头的行是注释，空行忽略；符号集（.inc）见
//       symbol-set.mjs。
// ------------------------------------------------------------------------
import { FcdevError } from '../../errors/fcdev-error';

/** 一行符号：名字、可选的等号、地址 */
const SYMBOL_LINE = /^([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|\s)\s*(?:\$|0[xX])?([0-9A-Fa-f]{1,4})\s*(?:[;#].*)?$/;

/**
 * 解析符号文件。
 * @param text 文件内容
 * @returns 符号列表，按文件里的顺序
 * @throws FcdevError 某一行格式不对
 */
export function parseSymbols(text: string): {name: string, address: number}[] {
  const symbols: Array<{name: string, address: number}> = [];
  text.split(/\r?\n/).forEach((rawLine, index) => {
    const line = rawLine.trim();
    if (line === '' || line.startsWith(';') || line.startsWith('#')) return;
    const match = SYMBOL_LINE.exec(line);
    if (!match) throw new FcdevError(`符号文件第 ${index + 1} 行格式不对：${line}`, { code: 'DISASM_SYMBOL_INVALID', hint: '写成“名字 = $地址”或“名字 地址”' });
    symbols.push({ name: match[1]!, address: Number.parseInt(match[2]!, 16) });
  });
  return symbols;
}
