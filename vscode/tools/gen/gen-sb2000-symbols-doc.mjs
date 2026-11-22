// ------------------------------------------------------------------------
// 名称：gen-sb2000-symbols-doc.mjs
// 说明：由 SB2000 目标的 sb2000.inc 生成符号参考文档 symbols-reference.md：按分节列出每个符号的汇编名、C 名、值与说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-18
// 备注：用法 node vscode/tools/gen/gen-sb2000-symbols-doc.mjs；改了 sb2000.inc 后重新运行；test/sb2000-symbols-doc.test.mjs 核对入库的文档与生成结果一致。
//       符号的说明只来自 sb2000.inc 的行尾注释，这里不手写任何符号说明；C 名按 gen-sb2000-header.mjs 的规则在汇编名前加 SB_。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseSymbolSections } from './gen-sb2000-header.mjs';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', '..');

/** SB2000 目标目录 */
const TARGET_DIR = path.join(REPO_ROOT, 'vscode', 'targets', 'sb2000');

/** 生成的文档路径 */
export const SYMBOLS_DOC_FILE = path.join(TARGET_DIR, 'symbols-reference.md');

/**
 * 转义表格单元格里的竖线。
 * @param {string} text 单元格文字
 * @returns {string} 转义后的文字
 */
function toCell(text) {
  return text.replace(/\|/g, '\\|');
}

/**
 * 生成符号参考文档的全文。
 * @param {string} incText sb2000.inc 的内容
 * @returns {string} Markdown 文本（LF 换行）
 */
export function renderSymbolsReference(incText) {
  const sections = parseSymbolSections(incText);
  const total = sections.reduce((sum, section) => sum + section.symbols.length, 0);
  const lines = [
    '# SB2000 符号参考',
    '',
    '本文由符号定义文件 `sb2000.inc` 生成。',
    `共 ${total} 个符号，${sections.length} 节：系统调用的功能号、子功能号、系统变量的地址、出错码、硬件寄存器与位、运行库入口。`,
    '',
    '用法：',
    '',
    '- 汇编程序用 `.include "sb2000.inc"` 引入，直接写“汇编名”；文件只含符号，不占空间，值不超过 `$FF` 的符号按零页寻址。',
    '- C 程序 `#include <sb2000.h>` 已包含 `sb2000-symbols.h`，名字是“C 名”：汇编名前加 `SB_`，值是同一个数。',
    '- 汇编程序调用运行库的 C 函数时，函数名前加下划线（如 `_sb_puts`），参数按 cc65 的 fastcall 约定传递；全部函数见 [SB2000 C 接口参考](api-reference.md)。',
    '',
    '## 目录',
    '',
  ];
  sections.forEach((section, index) => lines.push(`- [${section.title}](#s${String(index + 1).padStart(2, '0')})`));
  lines.push('');

  sections.forEach((section, index) => {
    lines.push(`<a id="s${String(index + 1).padStart(2, '0')}"></a>`, `## ${section.title}`, '');
    lines.push('| 汇编名 | C 名 | 值 | 说明 |', '| --- | --- | --- | --- |');
    for (const { name, value, comment } of section.symbols) {
      const digits = value.length > 2 ? value.padStart(4, '0') : value.padStart(2, '0');
      lines.push(`| \`${name}\` | \`SB_${name}\` | \`$${digits.toUpperCase()}\` | ${toCell(comment)} |`);
    }
    lines.push('');
  });
  return lines.join('\n');
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const text = renderSymbolsReference(readFileSync(path.join(TARGET_DIR, 'asminc', 'sb2000.inc'), 'utf8'));
  writeFileSync(SYMBOLS_DOC_FILE, text);
  console.log(`已生成 ${path.relative(REPO_ROOT, SYMBOLS_DOC_FILE)}（${text.split('\n').length} 行）`);
}
