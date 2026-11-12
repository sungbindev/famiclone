// ------------------------------------------------------------------------
// 名称：assembly-symbol-resolver.ts
// 说明：汇编符号解析器：解析并缓存文件，按“当前文件、包含文件、工作区其他文件”的顺序查找符号。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：当前文档用传入的文本解析（含未保存的修改），包含文件与工作区文件读自磁盘并缓存；
//       .include 先按当前文件所在目录找，再按项目的汇编引入目录找，最多跟随 8 层。
// ------------------------------------------------------------------------

import type { AssemblyDocument } from '../../contracts/assembly/assembly-intellisense-service';
import type { AssemblySourceStore } from '../../contracts/assembly/assembly-source-store';
import type { ProjectInfoProvider } from '../../contracts/project/project-info';
import type { ParsedAssembly, ParsedSymbol } from '../../shared/assembly/assembly-symbol';
import { parseAssembly } from '../../shared/assembly/ca65-parser';
import { dirnameOf, joinPath } from '../../shared/assembly/path-text';

/** 跟随 .include 的最大层数。 */
const MAX_INCLUDE_DEPTH = 8;

/** 带所在文件的符号。 */
export interface IndexedSymbol extends ParsedSymbol {
  /** 符号所在文件的绝对路径。 */
  readonly path: string;
}

/** 按可见范围分组的符号，靠前的组优先。 */
export interface VisibleSymbols {
  /** 当前文档里的符号，含局部标签。 */
  readonly own: readonly IndexedSymbol[];
  /** 经 .include 引入的符号，不含局部标签。 */
  readonly included: readonly IndexedSymbol[];
  /** 工作区其他文件里的符号，不含局部标签。 */
  readonly workspace: readonly IndexedSymbol[];
}

/** 给符号加上所在文件。 */
function index(path: string, parsed: ParsedAssembly, withLocals: boolean): IndexedSymbol[] {
  return parsed.symbols.filter((symbol) => withLocals || !symbol.isLocal).map((symbol) => ({ ...symbol, path }));
}

/** 汇编符号解析器。 */
export class AssemblySymbolResolver {
  private readonly parsedFiles = new Map<string, Promise<ParsedAssembly | undefined>>();
  private workspaceFiles: Promise<readonly string[]> | undefined;

  /**
   * 创建解析器。
   * @param store 汇编源文件的存取。
   * @param projects 项目信息的读取。
   */
  constructor(
    private readonly store: AssemblySourceStore,
    private readonly projects: ProjectInfoProvider
  ) {}

  /**
   * 丢弃某个文件的解析缓存与工作区文件列表。
   * @param path 文件的绝对路径。
   */
  invalidate(path: string): void {
    this.parsedFiles.delete(path);
    this.workspaceFiles = undefined;
  }

  /**
   * 找出名字对应的符号定义，按可见范围的先后排列。
   * @param document 正在编辑的文档。
   * @param name 符号名。
   * @returns 同一范围内的全部同名定义；找不到时为空数组。
   */
  async find(document: AssemblyDocument, name: string): Promise<readonly IndexedSymbol[]> {
    const own = index(document.path, parseAssembly(document.text), true).filter((symbol) => symbol.name === name);
    if (own.length > 0 || name.startsWith('@')) return own;
    const { included, workspace } = await this.visible(document);
    const fromIncludes = included.filter((symbol) => symbol.name === name);
    if (fromIncludes.length > 0) return fromIncludes;
    return workspace.filter((symbol) => symbol.name === name);
  }

  /**
   * 列出当前文档能看到的全部符号。
   * @param document 正在编辑的文档。
   * @returns 按范围分组的符号。
   */
  async visible(document: AssemblyDocument): Promise<VisibleSymbols> {
    const parsed = parseAssembly(document.text);
    const seen = new Set<string>([document.path]);
    const info = await this.projects.get(document.path);
    const includeDirs = info?.asmIncludeDirs ?? [];
    const included: IndexedSymbol[] = [];
    await this.collectIncludes(document.path, parsed, includeDirs, seen, included, 0);

    const workspace: IndexedSymbol[] = [];
    for (const path of await this.listWorkspaceFiles()) {
      if (seen.has(path)) continue;
      seen.add(path);
      const other = await this.parseFile(path);
      if (other) workspace.push(...index(path, other, false));
    }
    return { own: index(document.path, parsed, true), included, workspace };
  }

  /** 递归收集 .include 引入的符号。 */
  private async collectIncludes(
    fromPath: string,
    parsed: ParsedAssembly,
    includeDirs: readonly string[],
    seen: Set<string>,
    output: IndexedSymbol[],
    depth: number
  ): Promise<void> {
    if (depth >= MAX_INCLUDE_DEPTH) return;
    for (const include of parsed.includes) {
      const candidates = [joinPath(dirnameOf(fromPath), include.name), ...includeDirs.map((dir) => joinPath(dir, include.name))];
      for (const candidate of candidates) {
        if (seen.has(candidate)) break;
        const child = await this.parseFile(candidate);
        if (!child) continue;
        seen.add(candidate);
        output.push(...index(candidate, child, false));
        await this.collectIncludes(candidate, child, includeDirs, seen, output, depth + 1);
        break;
      }
    }
  }

  /** 读取并解析一个磁盘文件，结果按路径缓存。 */
  private parseFile(path: string): Promise<ParsedAssembly | undefined> {
    let parsed = this.parsedFiles.get(path);
    if (parsed === undefined) {
      parsed = this.store.read(path).then((text) => (text === undefined ? undefined : parseAssembly(text)));
      this.parsedFiles.set(path, parsed);
    }
    return parsed;
  }

  /** 工作区里的汇编文件列表，缓存到下一次失效。 */
  private listWorkspaceFiles(): Promise<readonly string[]> {
    this.workspaceFiles ??= this.store.listWorkspaceFiles();
    return this.workspaceFiles;
  }
}
