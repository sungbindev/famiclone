// ------------------------------------------------------------------------
// 名称：assembly-providers.test.ts
// 说明：测试汇编语言的入口层：四个提供者的数据转换与错误处理，以及注册时的选择器与缓存失效。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用 vscode 替身；服务用替身，只验证入口层“转换并委派”的行为。
// ------------------------------------------------------------------------

import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { AssemblyIntelliSenseService } from '../../src/contracts/assembly/assembly-intellisense-service';
import { mockState, resetMock } from '../support/vscode-mock';

vi.mock('vscode', () => import('../support/vscode-mock'));

const { AssemblyCompletionProvider, AssemblyDefinitionProvider, AssemblyDocumentSymbolProvider, AssemblyHoverProvider } = await import(
  '../../src/entry/assembly/assembly-providers'
);
const { registerAssemblyIntelliSense } = await import('../../src/entry/assembly/assembly-registration');

const RANGE = { start: { line: 1, character: 2 }, end: { line: 1, character: 9 } };

/** 编辑器文档的替身。 */
const DOCUMENT = { uri: { fsPath: 'd:\\proj\\main.s' }, getText: () => '_main:\n        rts' };

/** 位置的替身。 */
const POSITION = { line: 1, character: 10 };

/** 服务替身：每个方法都是可检查的 vi.fn。 */
function fakeService(overrides: Partial<AssemblyIntelliSenseService> = {}): AssemblyIntelliSenseService {
  return {
    hover: vi.fn(async () => ({ markdown: '**说明**', range: RANGE })),
    complete: vi.fn(async () => [{ label: 'SYSCALL', kind: 'constant' as const, detail: '常量 $FFF0', documentation: '入口' }]),
    definition: vi.fn(async () => ({ path: 'd:\\sdk\\sb2000.inc', range: RANGE })),
    documentSymbols: vi.fn(() => [{ name: '_main', kind: 'label' as const, detail: '', range: RANGE }]),
    invalidate: vi.fn(),
    ...overrides
  };
}

const logger = { info: vi.fn(), error: vi.fn() };

beforeEach(() => {
  resetMock();
  logger.error.mockClear();
});

describe('悬停提供者', () => {
  it('把服务的悬停转成编辑器的悬停，并传入文档与位置', async () => {
    const service = fakeService();
    const hover = (await new AssemblyHoverProvider(service, logger).provideHover(DOCUMENT as never, POSITION as never)) as never as {
      contents: { value: string };
      range: { startLine: number; endCharacter: number };
    };
    expect(hover.contents.value).toBe('**说明**');
    expect(hover.range).toMatchObject({ startLine: 1, endCharacter: 9 });
    expect(service.hover).toHaveBeenCalledWith({ path: 'd:\\proj\\main.s', text: '_main:\n        rts' }, POSITION);
  });

  it('没有结果时返回空；服务出错时记日志并返回空', async () => {
    const none = fakeService({ hover: vi.fn(async () => undefined) });
    expect(await new AssemblyHoverProvider(none, logger).provideHover(DOCUMENT as never, POSITION as never)).toBeUndefined();
    const broken = fakeService({ hover: vi.fn(async () => Promise.reject(new Error('坏了'))) });
    expect(await new AssemblyHoverProvider(broken, logger).provideHover(DOCUMENT as never, POSITION as never)).toBeUndefined();
    expect(logger.error).toHaveBeenCalledTimes(1);
  });
});

describe('补全提供者', () => {
  it('补全项转成编辑器的补全项：种类、说明与文档', async () => {
    const items = (await new AssemblyCompletionProvider(fakeService(), logger).provideCompletionItems(
      DOCUMENT as never,
      POSITION as never
    )) as never as Array<{ label: string; kind: number; detail: string; documentation: { value: string } }>;
    expect(items).toHaveLength(1);
    expect(items[0]).toMatchObject({ label: 'SYSCALL', kind: 20, detail: '常量 $FFF0', documentation: { value: '入口' } });
  });

  it('服务出错时记日志并返回空列表', async () => {
    const broken = fakeService({ complete: vi.fn(async () => Promise.reject(new Error('坏了'))) });
    expect(await new AssemblyCompletionProvider(broken, logger).provideCompletionItems(DOCUMENT as never, POSITION as never)).toEqual([]);
    expect(logger.error).toHaveBeenCalledTimes(1);
  });
});

describe('转到定义提供者', () => {
  it('转成指向定义文件与范围的位置，没有结果时返回空', async () => {
    const location = (await new AssemblyDefinitionProvider(fakeService(), logger).provideDefinition(
      DOCUMENT as never,
      POSITION as never
    )) as never as { uri: { fsPath: string }; range: { startLine: number } };
    expect(location.uri.fsPath).toBe('d:\\sdk\\sb2000.inc');
    expect(location.range.startLine).toBe(1);
    const none = fakeService({ definition: vi.fn(async () => undefined) });
    expect(await new AssemblyDefinitionProvider(none, logger).provideDefinition(DOCUMENT as never, POSITION as never)).toBeUndefined();
  });
});

describe('文档符号提供者', () => {
  it('转成大纲符号，种类按符号种类映射', () => {
    const symbols = new AssemblyDocumentSymbolProvider(fakeService(), logger).provideDocumentSymbols(DOCUMENT as never) as never as Array<{
      name: string;
      kind: number;
    }>;
    expect(symbols).toMatchObject([{ name: '_main', kind: 19 }]);
  });
});

describe('注册', () => {
  it('四个提供者都只对 ca65 语言的磁盘文件生效，补全由 . 与 @ 触发', () => {
    registerAssemblyIntelliSense({ subscriptions: [] } as never, fakeService(), logger);
    expect(mockState.providers.map((item) => item.kind).toSorted()).toEqual(['completion', 'definition', 'hover', 'symbols']);
    for (const item of mockState.providers) expect(item.selector).toEqual({ language: 'ca65', scheme: 'file' });
    expect(mockState.providers.find((item) => item.kind === 'completion')?.triggers).toEqual(['.', '@']);
  });

  it('汇编文件被修改、创建、删除时，缓存按文件路径失效', () => {
    const service = fakeService();
    registerAssemblyIntelliSense({ subscriptions: [] } as never, service, logger);
    const watcher = mockState.watchers.get('**/*.{s,asm,inc}');
    expect(watcher).toBeDefined();
    watcher?.changed.forEach((callback) => callback({ fsPath: 'd:\\a.s' } as never));
    watcher?.created.forEach((callback) => callback({ fsPath: 'd:\\b.inc' } as never));
    watcher?.deleted.forEach((callback) => callback({ fsPath: 'd:\\c.asm' } as never));
    expect(vi.mocked(service.invalidate).mock.calls).toEqual([['d:\\a.s'], ['d:\\b.inc'], ['d:\\c.asm']]);
  });
});
