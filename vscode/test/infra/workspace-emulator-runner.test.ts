// ------------------------------------------------------------------------
// 名称：workspace-emulator-runner.test.ts
// 说明：测试动态加载工作区里 fcdev 的模拟器模块：版本检查、模块状态在多次调用之间保持、
//       停用时只在用过模拟器的情况下收尾，以及对仓库里真实的 fcdev 查询状态。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：版本检查用临时目录里写的假 fcdev，每个场景一个子目录（Node 按文件地址缓存模块）；
//       端到端用例把仓库的 packages/fcdev 用目录联接放进临时工作区，不启动 Ember。
// ------------------------------------------------------------------------

import { mkdirSync, mkdtempSync, rmSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { WorkspaceEmulatorRunner } from '../../src/infra/disassembly/workspace-emulator-runner';

const REPOSITORY_FCDEV = path.resolve(__dirname, '..', '..', '..', 'packages', 'fcdev');
const logger = { info: vi.fn(), error: vi.fn() };

let root: string;

beforeEach(() => {
  root = mkdtempSync(path.join(tmpdir(), 'fcdev-emu-'));
  logger.info.mockClear();
  logger.error.mockClear();
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

/** 在 root 下的 scenario 子目录里写一个假的 fcdev 包，返回子目录。 */
function fakeFcdev(scenario: string, moduleSource: string): string {
  const base = path.join(root, scenario);
  const directory = path.join(base, 'node_modules', 'fcdev');
  mkdirSync(path.join(directory, 'src', 'emulator'), { recursive: true });
  writeFileSync(path.join(directory, 'package.json'), JSON.stringify({ name: 'fcdev', version: '9.9.9' }));
  writeFileSync(path.join(directory, 'src', 'emulator', 'index.mjs'), moduleSource);
  return base;
}

describe('加载与版本检查', () => {
  it('按操作名调用，模块里的状态在多次调用之间保持（会话不丢）', async () => {
    const base = fakeFcdev('state', `export const EMULATOR_API_VERSION = 1; let connected = false;
      export async function runEmulatorOperation(name) { if (name === 'connect') connected = true; return { name, connected }; }`);
    const runner = new WorkspaceEmulatorRunner(logger);
    expect(await runner.run(base, 'status', {})).toEqual({ name: 'status', connected: false });
    await runner.run(base, 'connect', {});
    expect(await runner.run(base, 'status', {})).toEqual({ name: 'status', connected: true });
  });

  it('接口版本过旧、过新或缺失时提示升级，说明里写模拟器', async () => {
    const old = fakeFcdev('old', 'export const EMULATOR_API_VERSION = 0; export async function runEmulatorOperation() { return 1; }');
    await expect(new WorkspaceEmulatorRunner(logger).run(old, 'status', {})).rejects.toThrow(/模拟器接口过旧.*升级 fcdev/);
    const newer = fakeFcdev('newer', 'export const EMULATOR_API_VERSION = 2; export async function runEmulatorOperation() { return 1; }');
    await expect(new WorkspaceEmulatorRunner(logger).run(newer, 'status', {})).rejects.toThrow(/9\.9\.9 的模拟器接口比本扩展新/);
    const missing = fakeFcdev('missing', 'export const something = 1;');
    await expect(new WorkspaceEmulatorRunner(logger).run(missing, 'status', {})).rejects.toThrow(/还没有模拟器工具接口/);
  });
});

describe('停用时收尾', () => {
  it('用过模拟器时关闭会话并结束自己启动的进程', async () => {
    const base = fakeFcdev('shutdown', `export const EMULATOR_API_VERSION = 1; export const calls = [];
      export async function runEmulatorOperation(name, params) { calls.push([name, params]); return { calls }; }`);
    const runner = new WorkspaceEmulatorRunner(logger);
    await runner.run(base, 'connect', {});
    await runner.shutdown();
    const final = (await runner.run(base, 'status', {})) as { calls: unknown[] };
    expect(final.calls).toEqual([['connect', {}], ['status', { close: true, terminate: true }], ['status', {}]]);
  });

  it('没有用过模拟器时什么也不做；收尾出错只记日志', async () => {
    await expect(new WorkspaceEmulatorRunner(logger).shutdown()).resolves.toBeUndefined();
    const base = fakeFcdev('shutdown-error', `export const EMULATOR_API_VERSION = 1;
      export async function runEmulatorOperation(name, params) { if (params.close) throw new TypeError('关不掉'); return {}; }`);
    const runner = new WorkspaceEmulatorRunner(logger);
    await runner.run(base, 'status', {});
    await expect(runner.shutdown()).resolves.toBeUndefined();
    expect(logger.error).toHaveBeenCalledTimes(1);
  });
});

describe('对仓库里真实的 fcdev', () => {
  it('没有连接时状态为未连接，其他操作提示怎么连接', async () => {
    const workspace = path.join(root, 'real');
    mkdirSync(path.join(workspace, 'node_modules'), { recursive: true });
    symlinkSync(REPOSITORY_FCDEV, path.join(workspace, 'node_modules', 'fcdev'), 'junction');
    const runner = new WorkspaceEmulatorRunner(logger);
    expect(await runner.run(workspace, 'status', {})).toEqual({ connected: false });
    await expect(runner.run(workspace, 'run', {})).rejects.toThrow(/还没有连接 Ember[\s\S]*建议：.*emu_launch/);
    await runner.shutdown();
  });
});
