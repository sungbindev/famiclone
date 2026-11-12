// ------------------------------------------------------------------------
// 名称：hover-text.ts
// 说明：生成悬停提示与补全说明的 Markdown 文字：符号、6502 指令、汇编指令三种。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：来自源码注释的文字按纯文本处理，先转义 Markdown 特殊字符，避免下划线、星号等被渲染成格式。
// ------------------------------------------------------------------------

import type { AssemblySymbolKind, ParsedSymbol } from './assembly-symbol';
import { SYMBOL_KIND_LABELS } from './assembly-symbol';
import type { DirectiveDoc } from './directive-reference';
import type { InstructionDoc } from './instruction-reference';
import { describeValue } from './number-literal';

/** 需要转义的 Markdown 特殊字符。 */
const MARKDOWN_SPECIALS = /[\\`*_[\]<>#|~]/g;

/** 转义文字里的 Markdown 特殊字符。 */
export function escapeMarkdown(text: string): string {
  return text.replace(MARKDOWN_SPECIALS, (char) => `\\${char}`);
}

/** 悬停提示里的符号信息：解析出的符号加上所在文件的显示名。 */
export interface SymbolHoverInput extends Pick<ParsedSymbol, 'name' | 'kind' | 'value' | 'documentation' | 'section' | 'line'> {
  /** 符号所在文件的显示名，通常是文件名。 */
  readonly fileLabel: string;
}

/** 符号定义的源码写法，放在悬停提示的代码块里。 */
function definitionLine(kind: AssemblySymbolKind, name: string, value: string): string {
  switch (kind) {
    case 'constant':
      return value === '' ? name : `${name} = ${value}`;
    case 'label':
      return `${name}:`;
    case 'define':
      return `.define ${name}${value === '' ? '' : ` ${value}`}`;
    case 'macro':
      return `.macro ${name}${value === '' ? '' : ` ${value}`}`;
    case 'procedure':
      return `.proc ${name}`;
    case 'scope':
      return `.scope ${name}`;
    case 'struct':
      return `.struct ${name}`;
    case 'enum':
      return `.enum ${name}`;
  }
}

/**
 * 生成符号的悬停提示。
 * @param symbol 符号与它所在文件的显示名。
 * @returns Markdown 文字。
 */
export function formatSymbolHover(symbol: SymbolHoverInput): string {
  const parts = ['```ca65', definitionLine(symbol.kind, symbol.name, symbol.value), '```'];
  const kindLabel = SYMBOL_KIND_LABELS[symbol.kind];
  const valueText = symbol.kind === 'constant' && symbol.value !== '' ? `，值 ${escapeMarkdown(describeValue(symbol.value))}` : '';
  const blocks = [parts.join('\n'), `${kindLabel}${valueText}`];
  if (symbol.documentation !== '') blocks.push(escapeMarkdown(symbol.documentation).replace(/\n/g, '  \n'));
  const source = `定义于 ${escapeMarkdown(symbol.fileLabel)} 第 ${symbol.line + 1} 行`;
  blocks.push(symbol.section === '' ? source : `分组：${escapeMarkdown(symbol.section)}\n\n${source}`);
  return blocks.join('\n\n');
}

/**
 * 生成 6502 指令的悬停提示。
 * @param doc 指令资料。
 * @returns Markdown 文字。
 */
export function formatInstructionHover(doc: InstructionDoc): string {
  const name = doc.mnemonic.toUpperCase();
  const rows = doc.modes.map((mode) => `| ${mode.name} | \`${doc.mnemonic}${mode.syntax === '' ? '' : ` ${mode.syntax}`}\` | ${mode.bytes} |`);
  return [
    `**${name}**：${doc.title}`,
    escapeMarkdown(doc.summary),
    `影响的标志位：${doc.flags}`,
    ['| 寻址方式 | 写法 | 字节 |', '| --- | --- | --- |', ...rows].join('\n')
  ].join('\n\n');
}

/**
 * 生成 ca65 汇编指令的悬停提示。
 * @param doc 指令资料。
 * @returns Markdown 文字。
 */
export function formatDirectiveHover(doc: DirectiveDoc): string {
  return [`**${doc.name}**`, ['```ca65', doc.syntax, '```'].join('\n'), escapeMarkdown(doc.summary)].join('\n\n');
}
