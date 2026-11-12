// ------------------------------------------------------------------------
// 名称：assembly-intellisense-service.ts
// 说明：汇编智能感知的服务接口与数据类型：悬停、补全、转到定义、文档符号。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：数据类型与 VS Code 无关，入口层负责转换成编辑器的类型；行号与列号都从 0 起。
// ------------------------------------------------------------------------

import type { AssemblySymbolKind } from '../../shared/assembly/assembly-symbol';

/** 文档里的一个位置。 */
export interface AssemblyPosition {
  /** 行号。 */
  readonly line: number;
  /** 列号。 */
  readonly character: number;
}

/** 文档里的一段范围，不含结束位置。 */
export interface AssemblyRange {
  /** 起点。 */
  readonly start: AssemblyPosition;
  /** 终点。 */
  readonly end: AssemblyPosition;
}

/** 正在编辑的汇编文档。 */
export interface AssemblyDocument {
  /** 文件的绝对路径。 */
  readonly path: string;
  /** 当前文本，可能含尚未保存的修改。 */
  readonly text: string;
}

/** 悬停提示。 */
export interface AssemblyHover {
  /** Markdown 正文。 */
  readonly markdown: string;
  /** 悬停的词所在的范围。 */
  readonly range: AssemblyRange;
}

/** 补全项的种类：符号种类，或汇编指令。 */
export type AssemblyCompletionKind = AssemblySymbolKind | 'mnemonic' | 'directive';

/** 一个补全项。 */
export interface AssemblyCompletion {
  /** 显示与插入的文字。 */
  readonly label: string;
  /** 种类。 */
  readonly kind: AssemblyCompletionKind;
  /** 简短描述，显示在补全列表里。 */
  readonly detail: string;
  /** Markdown 格式的详细说明。 */
  readonly documentation: string;
}

/** 符号定义的位置。 */
export interface AssemblyLocation {
  /** 定义所在文件的绝对路径。 */
  readonly path: string;
  /** 符号名所在的范围。 */
  readonly range: AssemblyRange;
}

/** 文档里的一个符号，用于大纲与跳转。 */
export interface AssemblyDocumentSymbol {
  /** 符号名。 */
  readonly name: string;
  /** 种类。 */
  readonly kind: AssemblySymbolKind;
  /** 附加说明，如常量的值。 */
  readonly detail: string;
  /** 符号名所在的范围。 */
  readonly range: AssemblyRange;
}

/** 汇编智能感知服务。 */
export interface AssemblyIntelliSenseService {
  /**
   * 取光标处的悬停提示。
   * @param document 正在编辑的文档。
   * @param position 光标位置。
   * @returns 悬停提示；光标处没有可说明的内容时为 undefined。
   */
  hover(document: AssemblyDocument, position: AssemblyPosition): Promise<AssemblyHover | undefined>;

  /**
   * 取补全项。
   * @param document 正在编辑的文档。
   * @param position 光标位置。
   * @returns 补全项；光标在注释或字符串里时为空数组。
   */
  complete(document: AssemblyDocument, position: AssemblyPosition): Promise<readonly AssemblyCompletion[]>;

  /**
   * 找出光标处符号的定义。
   * @param document 正在编辑的文档。
   * @param position 光标位置。
   * @returns 定义位置；找不到时为 undefined。
   */
  definition(document: AssemblyDocument, position: AssemblyPosition): Promise<AssemblyLocation | undefined>;

  /**
   * 列出文档里的符号。
   * @param document 正在编辑的文档。
   * @returns 按出现顺序排列的符号，局部标签不含在内。
   */
  documentSymbols(document: AssemblyDocument): readonly AssemblyDocumentSymbol[];

  /**
   * 通知某个汇编文件已改变、新建或删除，缓存的解析结果随之失效。
   * @param path 文件的绝对路径。
   */
  invalidate(path: string): void;
}
