// ------------------------------------------------------------------------
// 名称：resolving-assembly-intellisense-service.ts
// 说明：汇编智能感知服务的实现：悬停、补全、转到定义、文档符号，符号来自解析器，指令来自参考资料。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：悬停与定义先看词的角色：汇编指令查指令资料，指令位置的助记符查 6502 指令资料，
//       其余按符号名查找；找不到助记符资料时当作宏调用，按符号查。
// ------------------------------------------------------------------------

import type {
  AssemblyCompletion,
  AssemblyDocument,
  AssemblyDocumentSymbol,
  AssemblyHover,
  AssemblyIntelliSenseService,
  AssemblyLocation,
  AssemblyPosition,
  AssemblyRange
} from '../../contracts/assembly/assembly-intellisense-service';
import { SYMBOL_KIND_LABELS } from '../../shared/assembly/assembly-symbol';
import { parseAssembly } from '../../shared/assembly/ca65-parser';
import { DIRECTIVES, findDirective } from '../../shared/assembly/directive-reference';
import { basenameOf } from '../../shared/assembly/path-text';
import { formatDirectiveHover, formatInstructionHover, formatSymbolHover } from '../../shared/assembly/hover-text';
import { INSTRUCTIONS, findInstruction } from '../../shared/assembly/instruction-reference';
import type { LineToken } from '../../shared/assembly/line-tokens';
import { completionContextAt, findTokenAt } from '../../shared/assembly/line-tokens';
import type { AssemblySymbolResolver, IndexedSymbol } from './assembly-symbol-resolver';

/** 取出文档的某一行。 */
function lineOf(document: AssemblyDocument, line: number): string {
  return document.text.split(/\r?\n/)[line] ?? '';
}

/** 一行里的列区间转成文档范围。 */
function rangeOf(line: number, start: number, end: number): AssemblyRange {
  return { start: { line, character: start }, end: { line, character: end } };
}

/** 符号名所在的范围。 */
function symbolRange(symbol: IndexedSymbol): AssemblyRange {
  return rangeOf(symbol.line, symbol.startColumn, symbol.endColumn);
}

/** 汇编智能感知服务的实现。 */
export class ResolvingAssemblyIntelliSenseService implements AssemblyIntelliSenseService {
  /**
   * 创建服务。
   * @param resolver 符号解析器。
   */
  constructor(private readonly resolver: AssemblySymbolResolver) {}

  /** @inheritdoc */
  async hover(document: AssemblyDocument, position: AssemblyPosition): Promise<AssemblyHover | undefined> {
    const token = findTokenAt(lineOf(document, position.line), position.character);
    if (!token) return undefined;
    const range = rangeOf(position.line, token.start, token.end);
    const markdown = await this.describe(document, token);
    return markdown === undefined ? undefined : { markdown, range };
  }

  /** @inheritdoc */
  async complete(document: AssemblyDocument, position: AssemblyPosition): Promise<readonly AssemblyCompletion[]> {
    const context = completionContextAt(lineOf(document, position.line), position.character);
    if (context.insideCommentOrString) return [];
    const directives = DIRECTIVES.map<AssemblyCompletion>((doc) => ({
      label: doc.name,
      kind: 'directive',
      detail: doc.syntax,
      documentation: formatDirectiveHover(doc)
    }));
    if (context.prefix.startsWith('.')) return directives;

    const visible = await this.resolver.visible(document);
    const symbols = new Map<string, AssemblyCompletion>();
    for (const symbol of [...visible.own, ...visible.included, ...visible.workspace]) {
      if (symbols.has(symbol.name)) continue;
      symbols.set(symbol.name, {
        label: symbol.name,
        kind: symbol.kind,
        detail: `${SYMBOL_KIND_LABELS[symbol.kind]}${symbol.value === '' ? '' : ` ${symbol.value}`}`,
        documentation: formatSymbolHover({ ...symbol, fileLabel: basenameOf(symbol.path) })
      });
    }
    if (!context.atInstructionSlot) return [...symbols.values()];

    const instructions = INSTRUCTIONS.map<AssemblyCompletion>((doc) => ({
      label: doc.mnemonic,
      kind: 'mnemonic',
      detail: doc.title,
      documentation: formatInstructionHover(doc)
    }));
    const macros = [...symbols.values()].filter((item) => item.kind === 'macro');
    return [...instructions, ...directives, ...macros];
  }

  /** @inheritdoc */
  async definition(document: AssemblyDocument, position: AssemblyPosition): Promise<AssemblyLocation | undefined> {
    const token = findTokenAt(lineOf(document, position.line), position.character);
    if (!token || token.text.startsWith('.')) return undefined;
    const [symbol] = await this.resolver.find(document, token.text);
    return symbol ? { path: symbol.path, range: symbolRange(symbol) } : undefined;
  }

  /** @inheritdoc */
  documentSymbols(document: AssemblyDocument): readonly AssemblyDocumentSymbol[] {
    return parseAssembly(document.text)
      .symbols.filter((symbol) => !symbol.isLocal)
      .map((symbol) => ({
        name: symbol.name,
        kind: symbol.kind,
        detail: symbol.value,
        range: rangeOf(symbol.line, symbol.startColumn, symbol.endColumn)
      }));
  }

  /** @inheritdoc */
  invalidate(path: string): void {
    this.resolver.invalidate(path);
  }

  /** 生成光标处词的悬停正文：汇编指令、助记符、符号，依次尝试。 */
  private async describe(document: AssemblyDocument, token: LineToken): Promise<string | undefined> {
    if (token.text.startsWith('.')) {
      const directive = findDirective(token.text);
      return directive ? formatDirectiveHover(directive) : undefined;
    }
    if (token.role === 'instruction') {
      const instruction = findInstruction(token.text);
      if (instruction) return formatInstructionHover(instruction);
    }
    const [symbol] = await this.resolver.find(document, token.text);
    return symbol ? formatSymbolHover({ ...symbol, fileLabel: basenameOf(symbol.path) }) : undefined;
  }
}
