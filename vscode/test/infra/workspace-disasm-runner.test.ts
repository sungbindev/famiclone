// ------------------------------------------------------------------------
// 名称：workspace-disasm-runner.test.ts
// 说明：测试动态加载工作区里 fcdev 的反汇编模块：查找、版本检查、错误转换，
//       以及对仓库里真实的 fcdev 做新建、追踪与查看。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：版本检查用临时目录里写的假 fcdev，每个场景一个子目录（Node 按文件地址缓存模块）；
//       端到端用例把仓库的 packages/fcdev 用目录联接放进临时工作区，不需要 cc65。
// ------------------------------------------------------------------------

import { mkdirSync, mkdtempSync, rmSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { UserFacingError } from '../../src/shared/errors/user-facing-error';
import { WorkspaceDisasmRunner, locateFcdevPackage } from '../../src/infra/disassembly/workspace-disasm-runner';

const REPOSITORY_FCDEV = path.resolve(__dirname, '..', '..', '..', 'packages', 'fcdev');
const logger = { info: vi.fn(), error: vi.fn() };

let root: string;

beforeEach(() => {
  root = mkdtempSync(path.join(tmpdir(), 'fcdev-ext-'));
  logger.info.mockClear();
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

/** 在 root 下的 scenario 子目录里写一个假的 fcdev 包，返回子目录。 */
function fakeFcdev(scenario: string, moduleSource: string, version = '9.9.9'): string {
  const base = path.join(root, scenario);
  const directory = path.join(base, 'node_modules', 'fcdev');
  mkdirSync(path.join(directory, 'src', 'disasm'), { recursive: true });
  writeFileSync(path.join(directory, 'package.json'), JSON.stringify({ name: 'fcdev', version }));
  writeFileSync(path.join(directory, 'src', 'disasm', 'index.mjs'), moduleSource);
  return base;
}

/** 临时工作区：node_modules/fcdev 联接到仓库的 packages/fcdev，里面放一个小映像。 */
function realWorkspace(base: string): { projectDir: string; imageFile: string } {
  const workspace = path.join(base, 'real');
  mkdirSync(path.join(workspace, 'node_modules'), { recursive: true });
  symlinkSync(REPOSITORY_FCDEV, path.join(workspace, 'node_modules', 'fcdev'), 'junction');
  const imageFile = path.join(workspace, 'game.bin');
  writeFileSync(imageFile, Buffer.from([0x20, 0x0b, 0xc0, 0xa9, 0x05, 0x8d, 0x00, 0x20, 0x4c, 0x08, 0xc0, 0xa2, 0x10, 0xca, 0xd0, 0xfd, 0x60]));
  return { projectDir: path.join(workspace, 'project'), imageFile };
}

describe('查找 fcdev', () => {
  it('从起点向上找 node_modules/fcdev，找不到时为 undefined', () => {
    const base = fakeFcdev('find', 'export const API_VERSION = 1;');
    const nested = path.join(base, 'a', 'b');
    mkdirSync(nested, { recursive: true });
    expect(locateFcdevPackage(nested)).toBe(path.join(base, 'node_modules', 'fcdev'));
    expect(locateFcdevPackage(path.join(root, 'empty'))).toBeUndefined();
  });
});

describe('加载与版本检查', () => {
  it('没有安装 fcdev 时报用户能看懂的错误', async () => {
    await expect(new WorkspaceDisasmRunner(logger).run(path.join(root, 'x'), 'status', {})).rejects.toThrow(/没有安装 fcdev/);
  });

  it('按操作名调用，参数原样传入；同一个包只加载一次', async () => {
    const base = fakeFcdev('call', 'export const API_VERSION = 1; let calls = 0; export async function runOperation(name, params) { calls += 1; return { name, params, calls }; }');
    const runner = new WorkspaceDisasmRunner(logger);
    expect(await runner.run(base, 'trace', { projectDir: 'p' })).toEqual({ name: 'trace', params: { projectDir: 'p' }, calls: 1 });
    expect(await runner.run(base, 'view', {})).toMatchObject({ calls: 2 });
    expect(logger.info).toHaveBeenCalledTimes(1);
  });

  it('接口版本过旧、过新或缺失时提示升级', async () => {
    const old = fakeFcdev('old', 'export const API_VERSION = 0; export async function runOperation() { return 1; }');
    await expect(new WorkspaceDisasmRunner(logger).run(old, 'status', {})).rejects.toThrow(/过旧.*升级 fcdev/);
    const newer = fakeFcdev('newer', 'export const API_VERSION = 2; export async function runOperation() { return 1; }', '10.0.0');
    await expect(new WorkspaceDisasmRunner(logger).run(newer, 'status', {})).rejects.toThrow(/10\.0\.0 的反汇编接口比本扩展新/);
    const missing = fakeFcdev('missing', 'export const something = 1;');
    await expect(new WorkspaceDisasmRunner(logger).run(missing, 'status', {})).rejects.toThrow(/还没有反汇编工具接口/);
  });

  it('fcdev 的可预期错误转成带建议的 UserFacingError，其他错误原样抛出', async () => {
    const base = fakeFcdev('errors', `export const API_VERSION = 1;
      export async function runOperation(name) {
        if (name === 'known') { const error = new Error('参数不对'); error.name = 'FcdevError'; error.hint = '改一下'; throw error; }
        throw new TypeError('缺陷');
      }`);
    const runner = new WorkspaceDisasmRunner(logger);
    const known = await runner.run(base, 'known', {}).catch((error: unknown) => error);
    expect(known).toBeInstanceOf(UserFacingError);
    expect((known as Error).message).toBe('参数不对\n建议：改一下');
    await expect(runner.run(base, 'other', {})).rejects.toBeInstanceOf(TypeError);
  });

  it('模块加载失败时说明原因', async () => {
    const base = fakeFcdev('broken', 'export const API_VERSION = ;');
    await expect(new WorkspaceDisasmRunner(logger).run(base, 'status', {})).rejects.toThrow(/加载 fcdev 9\.9\.9 的反汇编模块失败/);
  });
});

describe('对仓库里真实的 fcdev（端到端）', () => {
  it('新建工程、追踪、查看例程；错误带建议', async () => {
    const { projectDir, imageFile } = realWorkspace(root);
    const runner = new WorkspaceDisasmRunner(logger);
    const created = (await runner.run(projectDir, 'open', { projectDir, image: imageFile, address: '$C000', entries: ['$C000'] })) as { created: boolean };
    expect(created.created).toBe(true);
    const traced = (await runner.run(projectDir, 'trace', { projectDir })) as { instructions: number; routines: number };
    expect(traced).toMatchObject({ instructions: 8, routines: 2 });
    const view = (await runner.run(projectDir, 'view', { projectDir, routine: 'sub_C00B' })) as { text: string };
    expect(view.text).toContain('bne loc_C00D');
    const error = await runner.run(projectDir, 'view', { projectDir, routine: 'nope' }).catch((reason: unknown) => reason);
    expect(error).toBeInstanceOf(UserFacingError);
    expect((error as Error).message).toContain('没有名为 nope 的例程');
    expect((error as Error).message).toContain('建议：');
  });
});
