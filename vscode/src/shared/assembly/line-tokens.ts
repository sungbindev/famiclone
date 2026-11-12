// ------------------------------------------------------------------------
// 名称：line-tokens.ts
// 说明：汇编行的词法辅助：分出注释与字符串，找出光标处的词，判断指令位置与补全上下文。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只做按行的轻量分析，不是完整的 ca65 词法分析；字符串里的分号不算注释起点。
// ------------------------------------------------------------------------

/** 一行里注释与字符串的位置。 */
export interface LineLayout {
  /** 注释起点（分号）的列；没有注释为 -1。 */
  readonly commentStart: number;
  /** 字符串（含引号）的列区间 [起, 止)。 */
  readonly strings: ReadonlyArray<readonly [number, number]>;
}

/** 词在行里的角色。 */
export type TokenRole = 'label' | 'instruction' | 'operand';

/** 光标处的词。 */
export interface LineToken {
  /** 词本身，指令以 `.` 开头，局部标签以 `@` 开头。 */
  readonly text: string;
  /** 起始列。 */
  readonly start: number;
  /** 结束列（不含）。 */
  readonly end: number;
  /** 角色：标签定义、指令位置（助记符或汇编指令）、操作数。 */
  readonly role: TokenRole;
}

/** 补全位置的上下文。 */
export interface CompletionContext {
  /** 光标前已输入的词前缀。 */
  readonly prefix: string;
  /** 词前缀的起始列。 */
  readonly prefixStart: number;
  /** 光标是否处在指令位置（行首或标签之后）。 */
  readonly atInstructionSlot: boolean;
  /** 光标是否在注释或字符串里，在里面不提供补全。 */
  readonly insideCommentOrString: boolean;
}

/** 词的正则：可带前导点（汇编指令）和前导 @（局部标签）。 */
const TOKEN_PATTERN = /\.?@?[A-Za-z_][A-Za-z0-9_]*/g;

/** 行首可选的标签定义：标识符加冒号，但不是 `::` 与 `:=`。 */
const LEADING_LABEL = /^\s*@?[A-Za-z_][A-Za-z0-9_]*\s*:(?![:=])/;

/**
 * 分析一行的注释与字符串位置。
 * @param line 一行文字。
 * @returns 注释起点与字符串区间。
 */
export function layoutLine(line: string): LineLayout {
  const strings: Array<readonly [number, number]> = [];
  let quote = '';
  let quoteStart = 0;
  for (let index = 0; index < line.length; index += 1) {
    const char = line.charAt(index);
    if (quote !== '') {
      if (char === quote) {
        strings.push([quoteStart, index + 1]);
        quote = '';
      }
    } else if (char === '"' || char === "'") {
      quote = char;
      quoteStart = index;
    } else if (char === ';') {
      return { commentStart: index, strings };
    }
  }
  // 没有闭合的引号：字符串延伸到行尾，光标在行尾时也算在字符串里。
  if (quote !== '') strings.push([quoteStart, line.length + 1]);
  return { commentStart: -1, strings };
}

/**
 * 去掉行尾注释。
 * @param line 一行文字。
 * @returns 代码部分与注释正文（不含分号，已去首尾空白）。
 */
export function splitComment(line: string): { code: string; comment: string } {
  const { commentStart } = layoutLine(line);
  if (commentStart < 0) return { code: line, comment: '' };
  return { code: line.slice(0, commentStart), comment: line.slice(commentStart + 1).trim() };
}

/** 位置是否在注释或字符串里。 */
function isInsideCommentOrString(layout: LineLayout, column: number): boolean {
  if (layout.commentStart >= 0 && column > layout.commentStart) return true;
  return layout.strings.some(([start, end]) => column > start && column < end);
}

/**
 * 判断词在代码里的角色。
 * @param code 去掉注释后的代码。
 * @param start 词的起始列。
 * @param end 词的结束列（不含）。
 */
function roleOf(code: string, start: number, end: number): TokenRole {
  const label = LEADING_LABEL.exec(code);
  if (label && start < label[0].length) return 'label';
  const before = label ? code.slice(label[0].length, start) : code.slice(0, start);
  if (before.trim() !== '') return 'operand';
  // 行首的词后面跟着等号是常量定义，不是指令。
  return /^\s*:?=/.test(code.slice(end)) ? 'operand' : 'instruction';
}

/**
 * 找出光标处的词。
 * @param line 一行文字。
 * @param character 光标所在的列。
 * @returns 光标处的词；光标不在词上，或在注释、字符串里时为 undefined。
 */
export function findTokenAt(line: string, character: number): LineToken | undefined {
  const layout = layoutLine(line);
  if (isInsideCommentOrString(layout, character)) return undefined;
  const code = layout.commentStart < 0 ? line : line.slice(0, layout.commentStart);
  for (const match of code.matchAll(TOKEN_PATTERN)) {
    const start = match.index;
    const end = start + match[0].length;
    if (character < start || character > end) continue;
    // 前一个字符是标识符字符时，这个点不是汇编指令的前导点。
    if (match[0].startsWith('.') && start > 0 && /[A-Za-z0-9_@]/.test(code.charAt(start - 1))) continue;
    return { text: match[0], start, end, role: roleOf(code, start, end) };
  }
  return undefined;
}

/**
 * 分析补全位置的上下文。
 * @param line 一行文字。
 * @param character 光标所在的列。
 * @returns 已输入的前缀、前缀起点、是否在指令位置，以及是否在注释或字符串里。
 */
export function completionContextAt(line: string, character: number): CompletionContext {
  const layout = layoutLine(line);
  const before = line.slice(0, character);
  const prefix = /[.@A-Za-z0-9_]*$/.exec(before)?.[0] ?? '';
  const prefixStart = character - prefix.length;
  const head = before.slice(0, prefixStart);
  const label = LEADING_LABEL.exec(head);
  const rest = label ? head.slice(label[0].length) : head;
  return {
    prefix,
    prefixStart,
    atInstructionSlot: rest.trim() === '',
    insideCommentOrString: isInsideCommentOrString(layout, character)
  };
}
