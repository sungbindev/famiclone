// ------------------------------------------------------------------------
// 名称：assembly-providers.ts
// 说明：汇编语言的四个编辑器提供者：悬停、补全、转到定义、文档符号，都只转换数据并委派给服务。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：服务抛出的异常记入日志，对编辑器按“没有结果”处理，不弹出错误提示。
// ------------------------------------------------------------------------

import * as vscode from 'vscode';
import type { AssemblyIntelliSenseService } from '../../contracts/assembly/assembly-intellisense-service';
import type { Logger } from '../../platform/logging/logger';
import { toAssemblyDocument, toCompletionItemKind, toSymbolKind, toVsCodeRange } from './assembly-conversion';

/** 悬停提供者。 */
export class AssemblyHoverProvider implements vscode.HoverProvider {
  /**
   * 创建提供者。
   * @param service 汇编智能感知服务。
   * @param logger 日志。
   */
  constructor(
    private readonly service: AssemblyIntelliSenseService,
    private readonly logger: Logger
  ) {}

  /** @inheritdoc */
  async provideHover(document: vscode.TextDocument, position: vscode.Position): Promise<vscode.Hover | undefined> {
    try {
      const hover = await this.service.hover(toAssemblyDocument(document), { line: position.line, character: position.character });
      if (!hover) return undefined;
      return new vscode.Hover(new vscode.MarkdownString(hover.markdown), toVsCodeRange(hover.range));
    } catch (error) {
      this.logger.error('汇编悬停提示出错', error);
      return undefined;
    }
  }
}

/** 补全提供者。 */
export class AssemblyCompletionProvider implements vscode.CompletionItemProvider {
  /**
   * 创建提供者。
   * @param service 汇编智能感知服务。
   * @param logger 日志。
   */
  constructor(
    private readonly service: AssemblyIntelliSenseService,
    private readonly logger: Logger
  ) {}

  /** @inheritdoc */
  async provideCompletionItems(document: vscode.TextDocument, position: vscode.Position): Promise<vscode.CompletionItem[]> {
    try {
      const items = await this.service.complete(toAssemblyDocument(document), { line: position.line, character: position.character });
      return items.map((item) => {
        const completion = new vscode.CompletionItem(item.label, toCompletionItemKind(item.kind));
        completion.detail = item.detail;
        completion.documentation = new vscode.MarkdownString(item.documentation);
        return completion;
      });
    } catch (error) {
      this.logger.error('汇编补全出错', error);
      return [];
    }
  }
}

/** 转到定义提供者。 */
export class AssemblyDefinitionProvider implements vscode.DefinitionProvider {
  /**
   * 创建提供者。
   * @param service 汇编智能感知服务。
   * @param logger 日志。
   */
  constructor(
    private readonly service: AssemblyIntelliSenseService,
    private readonly logger: Logger
  ) {}

  /** @inheritdoc */
  async provideDefinition(document: vscode.TextDocument, position: vscode.Position): Promise<vscode.Location | undefined> {
    try {
      const location = await this.service.definition(toAssemblyDocument(document), { line: position.line, character: position.character });
      return location ? new vscode.Location(vscode.Uri.file(location.path), toVsCodeRange(location.range)) : undefined;
    } catch (error) {
      this.logger.error('汇编转到定义出错', error);
      return undefined;
    }
  }
}

/** 文档符号提供者，用于大纲与“转到编辑器中的符号”。 */
export class AssemblyDocumentSymbolProvider implements vscode.DocumentSymbolProvider {
  /**
   * 创建提供者。
   * @param service 汇编智能感知服务。
   * @param logger 日志。
   */
  constructor(
    private readonly service: AssemblyIntelliSenseService,
    private readonly logger: Logger
  ) {}

  /** @inheritdoc */
  provideDocumentSymbols(document: vscode.TextDocument): vscode.DocumentSymbol[] {
    try {
      return this.service.documentSymbols(toAssemblyDocument(document)).map((symbol) => {
        const range = toVsCodeRange(symbol.range);
        return new vscode.DocumentSymbol(symbol.name, symbol.detail, toSymbolKind(symbol.kind), range, range);
      });
    } catch (error) {
      this.logger.error('汇编文档符号出错', error);
      return [];
    }
  }
}
