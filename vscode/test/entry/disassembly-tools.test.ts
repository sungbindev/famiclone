// ------------------------------------------------------------------------
// 名称：disassembly-tools.test.ts
// 说明：测试反汇编工具的入口层：注册、输入检查、路径规则、确认提示、结果整理与错误处理。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：用 vscode 替身；fcdev 与构建比较用替身，只验证入口层“检查、委派、整理”的行为；
//       工具的结构取自真实的 package.json。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { pathToFileURL } from 'node:url';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { BuildComparison } from '../../src/contracts/disassembly/build-comparer';
import { UserFacingError } from '../../src/shared/errors/user-facing-error';
import { mockState, resetMock } from '../support/vscode-mock';

vi.mock('vscode', () => import('../support/vscode-mock'));

const { DISASSEMBLY_TOOLS, registerDisassemblyTools } = await import('../../src/entry/disassembly/disassembly-tool-registration');

const MANIFEST = JSON.parse(readFileSync(path.resolve(__dirname, '..', '..', 'package.json'), 'utf8')) as { contributes: { languageModelTools: Array<{ name: string }> } };
const WORKSPACE = path.resolve(tmpdir(), 'fcdev-ws');
const PROJECT = path.join(WORKSPACE, 'proj');
const logger = { info: vi.fn(), error: vi.fn() };

/** 一个已注册的工具，只用到入口层调用的两个方法。 */
interface RegisteredTool {
  prepareInvocation(options: { input: Record<string, unknown> }): { invocationMessage: string; confirmationMessages?: { title: string; message: { value: string } } };
  invoke(options: { input: Record<string, unknown> }): Promise<{ content: Array<{ value: string }> }>;
}

/** 按名字取已注册的工具。 */
function registered(name: string): RegisteredTool {
  return mockState.tools.get(name) as RegisteredTool;
}

/** 注册后按名字取工具。 */
function setup(overrides: { run?: (start: string, operation: string, params: Record<string, unknown>) => Promise<unknown>; build?: () => Promise<BuildComparison> } = {}) {
  const runner = { run: vi.fn(overrides.run ?? (async () => ({ ok: true }))) };
  const comparer = { buildAndCompare: vi.fn(overrides.build ?? (async () => ({ identical: true, builtFile: 'a', builtSize: 1, imageSize: 1, firstDifference: null }))) };
  const context = { subscriptions: [] as unknown[], extension: { packageJSON: MANIFEST } };
  registerDisassemblyTools(context as never, { runner, comparer, logger });
  return { runner, comparer, tool: registered, context };
}

beforeEach(() => {
  resetMock();
  mockState.workspaceFolders = [{ uri: { fsPath: WORKSPACE } }];
  logger.error.mockClear();
});

describe('注册', () => {
  it('代码里的 18 个工具都在 package.json 里声明并注册，没有多余的声明', () => {
    const { context } = setup();
    const declared = MANIFEST.contributes.languageModelTools.map((tool) => tool.name);
    expect(new Set(DISASSEMBLY_TOOLS.map((spec) => spec.name))).toEqual(new Set(declared));
    expect(new Set(mockState.tools.keys())).toEqual(new Set(declared));
    expect(declared).toHaveLength(18);
    expect(context.subscriptions).toHaveLength(18);
  });

  it('每个工具的声明完整：引用名与工具名相同，参数是不允许多余字段的对象，有给模型的说明', () => {
    const tools = MANIFEST.contributes.languageModelTools as unknown as Array<Record<string, unknown> & { inputSchema: Record<string, unknown> }>;
    for (const tool of tools) {
      expect(tool.toolReferenceName, String(tool.name)).toBe(tool.name);
      expect(tool.inputSchema.type, String(tool.name)).toBe('object');
      expect(tool.inputSchema.additionalProperties, String(tool.name)).toBe(false);
      expect(String(tool.modelDescription).length, String(tool.name)).toBeGreaterThan(30);
      for (const required of (tool.inputSchema.required ?? []) as string[]) {
        expect(Object.keys(tool.inputSchema.properties as object), `${String(tool.name)}.${required}`).toContain(required);
      }
    }
  });

  it('代码里每个工具用到的操作，都是仓库里 fcdev 提供的操作（构建比较除外）', async () => {
    const disasm = (await import(pathToFileURL(path.resolve(__dirname, '..', '..', '..', 'packages', 'fcdev', 'src', 'disasm', 'index.mjs')).href)) as {
      OPERATION_NAMES: string[];
      API_VERSION: number;
    };
    const operations = DISASSEMBLY_TOOLS.map((spec) => spec.operation).filter((operation) => operation !== 'build');
    expect(new Set(operations)).toEqual(new Set(disasm.OPERATION_NAMES));
    expect(operations).toHaveLength(disasm.OPERATION_NAMES.length);
    expect(disasm.API_VERSION).toBe(1);
  });

  it('package.json 里没有声明的工具不注册，并记日志', () => {
    const runner = { run: vi.fn() };
    const context = { subscriptions: [] as unknown[], extension: { packageJSON: { contributes: { languageModelTools: [] } } } };
    registerDisassemblyTools(context as never, { runner, comparer: { buildAndCompare: vi.fn() }, logger });
    expect(mockState.tools.size).toBe(0);
    expect(logger.error).toHaveBeenCalledTimes(18);
  });
});

describe('调用', () => {
  it('只读工具：相对路径按第一个工作区文件夹解析，结果整理成文字', async () => {
    const { runner, tool } = setup({ run: async () => ({ total: 1, text: 'sub_C000:\n  rts' }) });
    const result = await tool('disasm_view').invoke({ input: { projectDir: 'proj', routine: 'sub_C000' } });
    expect(runner.run).toHaveBeenCalledWith(PROJECT, 'view', { projectDir: PROJECT, routine: 'sub_C000' });
    expect(result.content[0]?.value).toBe('{\n  "total": 1\n}\n\nsub_C000:\n  rts');
  });

  it('输入不合法时返回原因，不调用 fcdev', async () => {
    const { runner, tool } = setup();
    const result = await tool('disasm_list').invoke({ input: { projectDir: 'proj', category: 'bad', extra: 1 } });
    const text = result.content[0]?.value ?? '';
    expect(text).toContain('错误：输入不合法');
    expect(text).toContain('category 必须是');
    expect(text).toContain('不认识的参数 extra');
    expect(runner.run).not.toHaveBeenCalled();
  });

  it('工程目录在工作区之外时拒绝；映像在工作区之外时允许', async () => {
    const { runner, tool } = setup();
    const outsideDir = path.resolve(WORKSPACE, '..', 'elsewhere');
    expect((await tool('disasm_status').invoke({ input: { projectDir: outsideDir } })).content[0]?.value).toContain('必须在工作区里');
    expect(runner.run).not.toHaveBeenCalled();
    const outsideImage = path.resolve(WORKSPACE, '..', 'game.nes');
    await tool('disasm_open').invoke({ input: { projectDir: 'proj', image: outsideImage } });
    expect(runner.run).toHaveBeenCalledWith(PROJECT, 'open', { projectDir: PROJECT, image: outsideImage });
  });

  it('没有打开工作区文件夹时说明原因', async () => {
    mockState.workspaceFolders = [];
    const { tool } = setup();
    expect((await tool('disasm_status').invoke({ input: { projectDir: 'proj' } })).content[0]?.value).toContain('没有打开工作区文件夹');
  });

  it('符号集数组里的每个路径都换成绝对路径；disasm_reference 不给 projectDir 时从工作区根找 fcdev', async () => {
    const { runner, tool } = setup();
    await tool('disasm_open').invoke({ input: { projectDir: 'proj', image: 'game.bin', symbolSets: ['a.inc', 'sub/b.inc'] } });
    expect(runner.run).toHaveBeenCalledWith(PROJECT, 'open', {
      projectDir: PROJECT, image: path.join(WORKSPACE, 'game.bin'), symbolSets: [path.join(WORKSPACE, 'a.inc'), path.join(WORKSPACE, 'sub', 'b.inc')]
    });
    await tool('disasm_reference').invoke({ input: { query: 'lda' } });
    expect(runner.run).toHaveBeenLastCalledWith(WORKSPACE, 'reference', { query: 'lda' });
  });

  it('可预期的错误作为结果文字返回；其他错误记日志并给通用说明', async () => {
    const known = setup({ run: async () => Promise.reject(new UserFacingError('没有名为 x 的例程\n建议：先追踪')) });
    expect((await known.tool('disasm_routine').invoke({ input: { projectDir: 'proj', routine: 'x' } })).content[0]?.value).toBe('错误：没有名为 x 的例程\n建议：先追踪');
    expect(logger.error).not.toHaveBeenCalled();
    resetMock();
    mockState.workspaceFolders = [{ uri: { fsPath: WORKSPACE } }];
    const bug = setup({ run: async () => Promise.reject(new TypeError('缺陷')) });
    const text = (await bug.tool('disasm_status').invoke({ input: { projectDir: 'proj' } })).content[0]?.value ?? '';
    expect(text).toContain('disasm_status 执行出错（缺陷）');
    expect(logger.error).toHaveBeenCalledTimes(1);
  });

  it('disasm_build 经构建比较器运行，不走 fcdev 的反汇编操作', async () => {
    const { runner, comparer, tool } = setup({ build: async () => ({ identical: false, builtFile: 'x.nes', builtSize: 4, imageSize: 4, firstDifference: 2 }) });
    const result = await tool('disasm_build').invoke({ input: { projectDir: 'proj', buildPath: 'rebuilt' } });
    expect(comparer.buildAndCompare).toHaveBeenCalledWith(PROJECT, path.join(WORKSPACE, 'rebuilt'));
    expect(runner.run).not.toHaveBeenCalled();
    expect(result.content[0]?.value).toContain('"firstDifference": 2');
  });

  it('输入是 undefined 时按空对象处理，报缺少参数', async () => {
    const { tool } = setup();
    const result = await tool('disasm_status').invoke({ input: undefined as never });
    expect(result.content[0]?.value).toContain('缺少参数 projectDir');
  });
});

describe('确认提示', () => {
  it('只读工具只显示调用说明，不要求确认', () => {
    const { tool } = setup();
    const prepared = tool('disasm_view').prepareInvocation({ input: { projectDir: 'proj', address: '$C000' } });
    expect(prepared.invocationMessage).toContain('proj');
    expect(prepared.confirmationMessages).toBeUndefined();
  });

  it('会写工程或运行外部程序的工具要求确认，说明里写明目录', () => {
    const { tool } = setup();
    for (const [name, input] of [
      ['disasm_trace', { projectDir: 'proj' }],
      ['disasm_apply', { projectDir: 'proj', edits: [{ op: 'addEntry' }] }],
      ['disasm_generate', { projectDir: 'proj' }],
      ['disasm_verify', { projectDir: 'proj' }],
      ['disasm_build', { projectDir: 'proj', buildPath: 'r' }]
    ] as const) {
      const prepared = tool(name).prepareInvocation({ input });
      expect(prepared.confirmationMessages?.title, name).toContain('fcdev：');
      expect(prepared.confirmationMessages?.message.value, name).toContain('proj');
    }
  });

  it('同一个工具只在会写的参数下要求确认：open 带 image、layout 带 segments、symbols 的 import', () => {
    const { tool } = setup();
    expect(tool('disasm_open').prepareInvocation({ input: { projectDir: 'proj' } }).confirmationMessages).toBeUndefined();
    expect(tool('disasm_open').prepareInvocation({ input: { projectDir: 'proj', image: 'g.nes' } }).confirmationMessages).toBeDefined();
    expect(tool('disasm_layout').prepareInvocation({ input: { projectDir: 'proj' } }).confirmationMessages).toBeUndefined();
    expect(tool('disasm_layout').prepareInvocation({ input: { projectDir: 'proj', segments: [] } }).confirmationMessages).toBeDefined();
    expect(tool('disasm_symbols').prepareInvocation({ input: { projectDir: 'proj', action: 'search', query: 'a' } }).confirmationMessages).toBeUndefined();
    expect(tool('disasm_symbols').prepareInvocation({ input: { projectDir: 'proj', action: 'import', file: 'a.inc' } }).confirmationMessages).toBeDefined();
  });

  it('映像在工作区之外时，确认说明里写出这个路径', () => {
    const { tool } = setup();
    const outsideImage = path.resolve(WORKSPACE, '..', 'game.nes');
    const prepared = tool('disasm_open').prepareInvocation({ input: { projectDir: 'proj', image: outsideImage } });
    expect(prepared.confirmationMessages?.message.value).toContain('工作区之外');
    expect(prepared.confirmationMessages?.message.value).toContain(outsideImage);
  });

  it('路径不合法时照常给出确认，错误留到调用时返回', () => {
    const { tool } = setup();
    const outsideDir = path.resolve(WORKSPACE, '..', 'elsewhere');
    expect(tool('disasm_trace').prepareInvocation({ input: { projectDir: outsideDir } }).confirmationMessages).toBeDefined();
  });
});
