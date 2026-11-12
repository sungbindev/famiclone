// ------------------------------------------------------------------------
// 名称：assembly-conversion.ts
// 说明：汇编智能感知的数据与 VS Code 类型之间的转换：文档、范围、补全种类与符号种类。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：服务接口的数据与 VS Code 无关，转换只在入口层做。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { AssemblyCompletionKind, AssemblyDocument, AssemblyRange } from '../../contracts/assembly/assembly-intellisense-service';
import type { AssemblySymbolKind } from '../../shared/assembly/assembly-symbol';

/** 汇编语言的标识，与 package.json 里声明的语言一致。 */
export const ASSEMBLY_LANGUAGE_ID = 'ca65';

/** 补全项种类到 VS Code 图标种类。 */
const COMPLETION_KINDS: Readonly<Record<AssemblyCompletionKind, vscode.CompletionItemKind>> = {
  constant: vscode.CompletionItemKind.Constant,
  label: vscode.CompletionItemKind.Reference,
  procedure: vscode.CompletionItemKind.Function,
  macro: vscode.CompletionItemKind.Snippet,
  define: vscode.CompletionItemKind.Constant,
  scope: vscode.CompletionItemKind.Module,
  struct: vscode.CompletionItemKind.Struct,
  enum: vscode.CompletionItemKind.Enum,
  mnemonic: vscode.CompletionItemKind.Keyword,
  directive: vscode.CompletionItemKind.Keyword
};

/** 符号种类到大纲里的符号种类。 */
const SYMBOL_KINDS: Readonly<Record<AssemblySymbolKind, vscode.SymbolKind>> = {
  constant: vscode.SymbolKind.Constant,
  label: vscode.SymbolKind.Key,
  procedure: vscode.SymbolKind.Function,
  macro: vscode.SymbolKind.Event,
  define: vscode.SymbolKind.Constant,
  scope: vscode.SymbolKind.Namespace,
  struct: vscode.SymbolKind.Struct,
  enum: vscode.SymbolKind.Enum
};

/**
 * 把编辑器里的文档转成服务接口的文档。
 * @param document 编辑器里的文档。
 * @returns 路径与当前文本。
 */
export function toAssemblyDocument(document: vscode.TextDocument): AssemblyDocument {
  return { path: document.uri.fsPath, text: document.getText() };
}

/**
 * 把服务接口的范围转成编辑器的范围。
 * @param range 服务接口的范围。
 * @returns 编辑器的范围。
 */
export function toVsCodeRange(range: AssemblyRange): vscode.Range {
  return new vscode.Range(range.start.line, range.start.character, range.end.line, range.end.character);
}

/**
 * 补全项种类转成编辑器的图标种类。
 * @param kind 补全项种类。
 * @returns 编辑器的补全种类。
 */
export function toCompletionItemKind(kind: AssemblyCompletionKind): vscode.CompletionItemKind {
  return COMPLETION_KINDS[kind];
}

/**
 * 符号种类转成大纲里的符号种类。
 * @param kind 符号种类。
 * @returns 编辑器的符号种类。
 */
export function toSymbolKind(kind: AssemblySymbolKind): vscode.SymbolKind {
  return SYMBOL_KINDS[kind];
}
