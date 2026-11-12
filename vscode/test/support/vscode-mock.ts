// ------------------------------------------------------------------------
// 名称：vscode-mock.ts
// 说明：vscode 模块的测试替身：只实现入口层用到的类与接口，并导出可控制的状态与记录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法：vi.mock('vscode', () => import('../support/vscode-mock'))；每个用例前调用 resetMock。
// ------------------------------------------------------------------------

import { vi } from 'vitest';

/** 可释放的资源。 */
export class Disposable {
  /** @param callback 释放时调用。 */
  constructor(private readonly callback: () => void = () => {}) {}

  /** 释放。 */
  dispose(): void {
    this.callback();
  }
}

/** 范围。 */
export class Range {
  /**
   * @param startLine 起始行。
   * @param startCharacter 起始列。
   * @param endLine 结束行。
   * @param endCharacter 结束列。
   */
  constructor(
    readonly startLine: number,
    readonly startCharacter: number,
    readonly endLine: number,
    readonly endCharacter: number
  ) {}
}

/** Markdown 文字。 */
export class MarkdownString {
  /** @param value 正文。 */
  constructor(readonly value: string = '') {}
}

/** 悬停。 */
export class Hover {
  /**
   * @param contents 正文。
   * @param range 范围。
   */
  constructor(
    readonly contents: MarkdownString,
    readonly range?: Range
  ) {}
}

/** 补全项。 */
export class CompletionItem {
  detail?: string;
  documentation?: MarkdownString;

  /**
   * @param label 显示文字。
   * @param kind 种类。
   */
  constructor(
    readonly label: string,
    readonly kind?: number
  ) {}
}

/** 文档符号。 */
export class DocumentSymbol {
  /**
   * @param name 名称。
   * @param detail 说明。
   * @param kind 种类。
   * @param range 范围。
   * @param selectionRange 选中范围。
   */
  constructor(
    readonly name: string,
    readonly detail: string,
    readonly kind: number,
    readonly range: Range,
    readonly selectionRange: Range
  ) {}
}

/** 资源标识。 */
export class Uri {
  /** @param fsPath 文件路径。 */
  constructor(readonly fsPath: string) {}

  /**
   * 由文件路径创建。
   * @param path 文件路径。
   */
  static file(path: string): Uri {
    return new Uri(path);
  }
}

/** 位置。 */
export class Location {
  /**
   * @param uri 文件。
   * @param range 范围。
   */
  constructor(
    readonly uri: Uri,
    readonly range: Range
  ) {}
}

export const CompletionItemKind = { Constant: 20, Reference: 17, Function: 2, Snippet: 14, Module: 8, Struct: 21, Enum: 12, Keyword: 13 };
export const SymbolKind = { Constant: 13, Key: 19, Function: 11, Event: 23, Namespace: 2, Struct: 22, Enum: 9 };
export const ProgressLocation = { Notification: 15 };

/** 文件监视器替身：保存订阅的回调，测试里手动触发。 */
export class FakeWatcher {
  readonly changed: Array<(uri: Uri) => void> = [];
  readonly created: Array<(uri: Uri) => void> = [];
  readonly deleted: Array<(uri: Uri) => void> = [];

  /** 订阅修改事件。 */
  onDidChange = (callback: (uri: Uri) => void): Disposable => {
    this.changed.push(callback);
    return new Disposable();
  };

  /** 订阅创建事件。 */
  onDidCreate = (callback: (uri: Uri) => void): Disposable => {
    this.created.push(callback);
    return new Disposable();
  };

  /** 订阅删除事件。 */
  onDidDelete = (callback: (uri: Uri) => void): Disposable => {
    this.deleted.push(callback);
    return new Disposable();
  };

  /** 释放。 */
  dispose(): void {}
}

/** 测试可控制的状态与记录。 */
export const mockState = {
  settings: {} as Record<string, unknown>,
  commands: new Map<string, (...args: unknown[]) => unknown>(),
  watchers: new Map<string, FakeWatcher>(),
  providers: [] as Array<{ kind: string; selector: unknown; provider: unknown; triggers: string[] }>,
  workspaceFolderListeners: [] as Array<() => void>,
  installedExtensions: new Set<string>()
};

/** 每个用例前重置状态与记录。 */
export function resetMock(): void {
  mockState.settings = {};
  mockState.commands.clear();
  mockState.watchers.clear();
  mockState.providers = [];
  mockState.workspaceFolderListeners = [];
  mockState.installedExtensions.clear();
  window.showInformationMessage.mockReset();
  window.showInformationMessage.mockResolvedValue(undefined);
  window.showWarningMessage.mockClear();
  window.showErrorMessage.mockClear();
  window.withProgress.mockClear();
  commands.executeCommand.mockClear();
}

/** 注册一类提供者，记录下来供测试取用。 */
function registerProvider(kind: string) {
  return (selector: unknown, provider: unknown, ...triggers: string[]): Disposable => {
    mockState.providers.push({ kind, selector, provider, triggers });
    return new Disposable();
  };
}

export const window = {
  showInformationMessage: vi.fn<(message: string, ...items: string[]) => Promise<string | undefined>>(async () => undefined),
  showWarningMessage: vi.fn(async () => undefined),
  showErrorMessage: vi.fn(async () => undefined),
  withProgress: vi.fn(async (_options: unknown, task: () => Promise<unknown>) => task())
};

export const commands = {
  registerCommand: (id: string, handler: (...args: unknown[]) => unknown): Disposable => {
    mockState.commands.set(id, handler);
    return new Disposable();
  },
  executeCommand: vi.fn(async (_id: string): Promise<unknown> => undefined)
};

export const extensions = {
  getExtension: (id: string): { id: string } | undefined => (mockState.installedExtensions.has(id) ? { id } : undefined)
};

export const languages = {
  registerHoverProvider: registerProvider('hover'),
  registerCompletionItemProvider: registerProvider('completion'),
  registerDefinitionProvider: registerProvider('definition'),
  registerDocumentSymbolProvider: registerProvider('symbols')
};

export const workspace = {
  getConfiguration: (section: string) => ({
    get: <T>(key: string, defaultValue: T): T => (mockState.settings[`${section}.${key}`] as T | undefined) ?? defaultValue
  }),
  createFileSystemWatcher: (pattern: string): FakeWatcher => {
    const watcher = new FakeWatcher();
    mockState.watchers.set(pattern, watcher);
    return watcher;
  },
  onDidChangeWorkspaceFolders: (callback: () => void): Disposable => {
    mockState.workspaceFolderListeners.push(callback);
    return new Disposable();
  }
};
