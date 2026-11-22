// ------------------------------------------------------------------------
// 名称：gen-sb2000-header.mjs
// 说明：从 SB2000 目标的 sb2000.inc 生成 C 头文件 sb2000-symbols.h：每个符号对应一个 SB_ 前缀的数值宏。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node tools/gen-sb2000-header.mjs；符号以 NAME = $值 的行定义，分节注释与行尾注释转成 C 注释；
//       改动 sb2000.inc 后重新运行，测试会检查入库的头文件与生成结果一致。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

/** SB2000 目标目录 */
const TARGET_DIR = path.join(REPO_ROOT, 'vscode', 'targets', 'sb2000');

/** 符号定义行：NAME = $HEX，可带行尾注释 */
const SYMBOL_LINE = /^([A-Z][A-Z0-9_]*)\s*=\s*\$([0-9A-Fa-f]+)\s*(?:;\s*(.*))?$/;

/** 分节注释行：; ---- 标题 ---- */
const SECTION_LINE = /^;\s*-{2,}\s*([^-\s].*?)\s*-{2,}\s*$/;

/**
 * 把注释文本转成安全的 C 注释内容。
 * @param {string} text 注释文本
 * @returns {string} 不含注释结束符的文本
 */
function toCommentText(text) {
  return text.replaceAll('*/', '* /').trim();
}

/**
 * 把 sb2000.inc 按分节注释拆成符号清单，没有符号的分节不收。
 * @param {string} incText sb2000.inc 的内容
 * @returns {{title: string, symbols: {name: string, value: string, comment: string}[]}[]} 分节清单，符号的值是不带 $ 的十六进制原文
 */
export function parseSymbolSections(incText) {
  const sections = [];
  let current = null;
  for (const rawLine of incText.split(/\r?\n/)) {
    const line = rawLine.trimEnd();
    const section = SECTION_LINE.exec(line);
    const symbol = SYMBOL_LINE.exec(line);
    if (section) {
      current = { title: section[1], symbols: [] };
      sections.push(current);
    } else if (symbol && current) {
      current.symbols.push({ name: symbol[1], value: symbol[2], comment: symbol[3] ?? '' });
    }
  }
  return sections.filter((item) => item.symbols.length > 0);
}

/**
 * 由 sb2000.inc 的内容生成头文件内容。
 * @param {string} incText sb2000.inc 的内容
 * @returns {string} sb2000-symbols.h 的内容
 */
export function generateSymbolsHeader(incText) {
  const body = [];
  for (const rawLine of incText.split(/\r?\n/)) {
    const line = rawLine.trimEnd();
    const section = SECTION_LINE.exec(line);
    const symbol = SYMBOL_LINE.exec(line);
    if (section) {
      body.push('', `/* ${toCommentText(section[1])} */`);
    } else if (symbol) {
      const [, name, value, comment] = symbol;
      const digits = value.length > 2 ? value.padStart(4, '0') : value.padStart(2, '0');
      const macro = `#define SB_${name} 0x${digits.toUpperCase()}`;
      body.push(comment ? `/** ${toCommentText(comment)} */\n${macro}` : macro);
    }
  }
  return `/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-symbols.h
 * 说明：SB-DOS 程序接口的符号常量：系统调用变量地址、功能号、子功能号、出错码、文件打开方式、运行库入口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-10
 * 备注：由 tools/gen-sb2000-header.mjs 从 asminc/sb2000.inc 生成，不要手工修改（DO NOT EDIT）；注释里的符号名省略 SB_ 前缀。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_SYMBOLS_H
#define SB2000_SYMBOLS_H
${body.join('\n')}

#endif
`;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const incText = readFileSync(path.join(TARGET_DIR, 'asminc', 'sb2000.inc'), 'utf8');
  const header = generateSymbolsHeader(incText);
  writeFileSync(path.join(TARGET_DIR, 'include', 'sb2000-symbols.h'), header);
  console.log(`已生成 sb2000-symbols.h：${(header.match(/#define SB_/g) ?? []).length} 个符号`);
}
