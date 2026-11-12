// ------------------------------------------------------------------------
// 名称：ca65-parser.ts
// 说明：解析 ca65 汇编源码：找出常量、标签、过程、宏、定义等符号和 .include 指令，并收集说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：按行解析，不展开宏与条件汇编；说明取符号所在行的行尾注释，没有时取紧贴在上方的
//       注释块；分组标题（“; ---- 标题 ----”）与分隔线不算说明，只记为符号的所属分组。
// ------------------------------------------------------------------------

import type { AssemblySymbolKind, ParsedAssembly, ParsedInclude, ParsedSymbol } from './assembly-symbol';
import { splitComment } from './line-tokens';

/** 标识符。 */
const IDENTIFIER = '@?[A-Za-z_][A-Za-z0-9_]*';

/** `名字 = 表达式` 或 `名字 := 表达式`。 */
const ASSIGNMENT = new RegExp(`^(${IDENTIFIER})\\s*:?=\\s*(.*)$`);

/** 标签：标识符加冒号，冒号后可以接指令，但不是 `::` 与 `:=`。 */
const LABEL = new RegExp(`^(${IDENTIFIER})\\s*:(?![:=])\\s*(.*)$`);

/** 定义符号的汇编指令：指令名到符号种类。 */
const NAMED_DIRECTIVES: Readonly<Record<string, AssemblySymbolKind>> = {
  '.define': 'define',
  '.macro': 'macro',
  '.mac': 'macro',
  '.proc': 'procedure',
  '.scope': 'scope',
  '.struct': 'struct',
  '.enum': 'enum'
};

/** 定义符号的汇编指令的写法：指令、名字、其后的文字（宏参数、定义的值）。 */
const NAMED_DIRECTIVE = new RegExp(`^(\\.[A-Za-z]+)\\s+(${IDENTIFIER})(.*)$`);

/** `.include "文件名"`。 */
const INCLUDE = /^\.include\s+"([^"]+)"/i;

/** 分组标题：`; ---- 标题 ----`。 */
const SECTION_HEADING = /^;\s*-{2,}\s*([^-\s].*?)\s*-{2,}\s*$/;

/** 分隔线：只有分隔字符的注释行。 */
const SEPARATOR_LINE = /^;\s*[-=*_#]{3,}\s*$/;

/** 一行文字是否只有注释（可带缩进）。 */
function isCommentOnly(line: string): boolean {
  return line.trimStart().startsWith(';');
}

/** 注释行是否是分组标题或分隔线，这两种不当作符号的说明。 */
function isDecoration(line: string): boolean {
  const text = line.trim();
  return SECTION_HEADING.test(text) || SEPARATOR_LINE.test(text);
}

/**
 * 取紧贴在第 index 行上方的注释块。
 * @param lines 全部行。
 * @param index 符号所在的行。
 * @returns 注释块的正文，每行去掉分号与首尾空白，行之间用换行连接；没有则为空字符串。
 */
function leadingComment(lines: readonly string[], index: number): string {
  const block: string[] = [];
  for (let cursor = index - 1; cursor >= 0; cursor -= 1) {
    const line = lines[cursor] ?? '';
    if (!isCommentOnly(line) || isDecoration(line)) break;
    block.unshift(line.trim().replace(/^;+\s?/, '').trim());
  }
  return block.join('\n').trim();
}

/**
 * 解析汇编源码。
 * @param text 源码全文。
 * @returns 符号与包含文件。
 */
export function parseAssembly(text: string): ParsedAssembly {
  const lines = text.split(/\r?\n/);
  const symbols: ParsedSymbol[] = [];
  const includes: ParsedInclude[] = [];
  let section = '';

  lines.forEach((line, index) => {
    const heading = SECTION_HEADING.exec(line.trim());
    if (heading) {
      section = heading[1] ?? '';
      return;
    }
    const { code, comment } = splitComment(line);
    const indent = code.length - code.trimStart().length;
    const body = code.trim();
    if (body === '') return;

    const include = INCLUDE.exec(body);
    if (include) {
      includes.push({ name: include[1] ?? '', line: index });
      return;
    }

    const add = (name: string, kind: AssemblySymbolKind, value: string, trailing: string): void => {
      const startColumn = line.indexOf(name, indent);
      symbols.push({
        name,
        kind,
        value: value.trim(),
        documentation: trailing !== '' ? trailing : leadingComment(lines, index),
        section,
        line: index,
        startColumn,
        endColumn: startColumn + name.length,
        isLocal: name.startsWith('@')
      });
    };

    const named = NAMED_DIRECTIVE.exec(body);
    const kind = named ? NAMED_DIRECTIVES[(named[1] ?? '').toLowerCase()] : undefined;
    if (named && kind) {
      add(named[2] ?? '', kind, named[3] ?? '', comment);
      return;
    }

    const assignment = ASSIGNMENT.exec(body);
    if (assignment) {
      add(assignment[1] ?? '', 'constant', assignment[2] ?? '', comment);
      return;
    }

    const label = LABEL.exec(body);
    if (label) {
      // 标签后面还有指令时，行尾注释属于那条指令，不当作标签的说明。
      const hasCode = (label[2] ?? '').trim() !== '';
      add(label[1] ?? '', 'label', '', hasCode ? '' : comment);
    }
  });

  return { symbols, includes };
}
