// ------------------------------------------------------------------------
// 名称：assembly-symbol.ts
// 说明：汇编符号的数据类型：解析器产出的符号、包含文件与解析结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：行号与列号都从 0 起，与 VS Code 一致；不含符号所在的文件，文件由使用方补上。
// ------------------------------------------------------------------------

/** 符号的种类。 */
export type AssemblySymbolKind =
  | 'constant'
  | 'label'
  | 'procedure'
  | 'macro'
  | 'define'
  | 'scope'
  | 'struct'
  | 'enum';

/** 符号种类的中文名，用于悬停提示与补全。 */
export const SYMBOL_KIND_LABELS: Readonly<Record<AssemblySymbolKind, string>> = {
  constant: '常量',
  label: '标签',
  procedure: '过程',
  macro: '宏',
  define: '定义',
  scope: '作用域',
  struct: '结构',
  enum: '枚举'
};

/** 解析出的一个符号。 */
export interface ParsedSymbol {
  /** 符号名，局部标签带 `@` 前缀。 */
  readonly name: string;
  /** 种类。 */
  readonly kind: AssemblySymbolKind;
  /** 值：常量与定义是等号右边的表达式，宏是参数表，其他种类为空字符串。 */
  readonly value: string;
  /** 说明：行尾注释，或紧贴在上方的注释块；没有则为空字符串。 */
  readonly documentation: string;
  /** 所属分组：上方最近一条“; ---- 标题 ----”分组标题的标题；没有则为空字符串。 */
  readonly section: string;
  /** 符号名所在的行。 */
  readonly line: number;
  /** 符号名的起始列。 */
  readonly startColumn: number;
  /** 符号名的结束列（不含）。 */
  readonly endColumn: number;
  /** 是否是局部标签，只在所属文件里可见。 */
  readonly isLocal: boolean;
}

/** 一条 `.include` 指令。 */
export interface ParsedInclude {
  /** 引号里的文件名。 */
  readonly name: string;
  /** 指令所在的行。 */
  readonly line: number;
}

/** 一份汇编源码的解析结果。 */
export interface ParsedAssembly {
  /** 按出现顺序排列的符号。 */
  readonly symbols: readonly ParsedSymbol[];
  /** 按出现顺序排列的包含文件。 */
  readonly includes: readonly ParsedInclude[];
}
