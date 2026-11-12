// ------------------------------------------------------------------------
// 名称：fcdev-project-info-provider.test.ts
// 说明：测试项目信息提供者：按 fcdev.json 找项目、缓存、失效、失败后的重试与日志。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：命令行用替身；项目目录与 fcdev.json 在临时目录里真实创建；用假定时器推进重试间隔。
// ------------------------------------------------------------------------

import { mkdirSync, mkdtempSync, realpathSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { FcdevCommandResult, FcdevCommandRunner } from '../../src/contracts/project/fcdev-command-runner';
import { FcdevProjectInfoProvider, findProjectRoot } from '../../src/infra/project/fcdev-project-info-provider';
import type { Logger } from '../../src/platform/logging/logger';
import { UserFacingError } from '../../src/shared/errors/user-facing-error';

/** 一份合法的 fcdev info --json 输出。 */
const INFO_JSON = JSON.stringify({
  root: 'x',
  target: { name: 'nes' },
  includeDirs: ['inc'],
  asmIncludeDirs: ['asminc']
});

/** 记录调用次数的命令行替身。 */
class CountingRunner implements FcdevCommandRunner {
  calls = 0;

  /** @param next 每次调用的结果或错误。 */
  constructor(private readonly next: () => FcdevCommandResult | Error) {}

  /** @inheritdoc */
  async run(): Promise<FcdevCommandResult> {
    this.calls += 1;
    const outcome = this.next();
    if (outcome instanceof Error) throw outcome;
    return outcome;
  }
}

/** 收集日志的替身。 */
function fakeLogger(): Logger & { infos: string[]; errors: string[] } {
  const infos: string[] = [];
  const errors: string[] = [];
  return { infos, errors, info: (message) => infos.push(message), error: (message) => errors.push(message) };
}

let temp = '';
let file = '';

beforeEach(() => {
  temp = realpathSync(mkdtempSync(path.join(tmpdir(), 'fcdev-ext-')));
  mkdirSync(path.join(temp, 'src'));
  writeFileSync(path.join(temp, 'fcdev.json'), '{}');
  file = path.join(temp, 'src', 'main.s');
});

afterEach(() => {
  vi.useRealTimers();
  rmSync(temp, { recursive: true, force: true });
});

describe('找项目', () => {
  it('从文件所在目录向上找 fcdev.json，没有时为空', () => {
    expect(findProjectRoot(file)).toBe(temp);
    expect(findProjectRoot(path.join(tmpdir(), 'fcdev-ext-no-project', 'a.s'))).toBeUndefined();
  });
});

describe('读取项目信息', () => {
  it('成功时返回解析后的信息，同一项目只运行一次命令', async () => {
    const runner = new CountingRunner(() => ({ exitCode: 0, stdout: INFO_JSON, stderr: '' }));
    const provider = new FcdevProjectInfoProvider(runner, fakeLogger());
    expect(await provider.get(file)).toMatchObject({ targetName: 'nes', asmIncludeDirs: ['asminc'] });
    await provider.get(path.join(temp, 'other.s'));
    expect(runner.calls).toBe(1);
  });

  it('文件不在 fcdev 项目里时不运行命令', async () => {
    const runner = new CountingRunner(() => ({ exitCode: 0, stdout: INFO_JSON, stderr: '' }));
    const provider = new FcdevProjectInfoProvider(runner, fakeLogger());
    expect(await provider.get(path.join(tmpdir(), 'fcdev-ext-no-project', 'a.s'))).toBeUndefined();
    expect(runner.calls).toBe(0);
  });

  it('失效后重新读取', async () => {
    const runner = new CountingRunner(() => ({ exitCode: 0, stdout: INFO_JSON, stderr: '' }));
    const provider = new FcdevProjectInfoProvider(runner, fakeLogger());
    await provider.get(file);
    provider.invalidate(temp);
    await provider.get(file);
    provider.invalidate();
    await provider.get(file);
    expect(runner.calls).toBe(3);
  });
});

describe('读取失败', () => {
  it('命令失败、输出不合法、可预期的错误都返回空并记日志，未预期的错误记为错误', async () => {
    const outcomes: Array<FcdevCommandResult | Error> = [
      { exitCode: 1, stdout: '', stderr: '错误：找不到工具链' },
      { exitCode: 0, stdout: '不是 JSON', stderr: '' },
      new UserFacingError('项目里没有安装 fcdev'),
      new Error('代码缺陷')
    ];
    const logger = fakeLogger();
    const provider = new FcdevProjectInfoProvider(new CountingRunner(() => outcomes.shift() ?? new Error('多余的调用')), logger);
    for (let attempt = 0; attempt < 4; attempt += 1) {
      provider.invalidate();
      expect(await provider.get(file)).toBeUndefined();
    }
    expect(logger.infos).toHaveLength(3);
    expect(logger.infos[0]).toContain('错误：找不到工具链');
    expect(logger.errors).toHaveLength(1);
  });

  it('失败的结果在 30 秒内沿用，之后允许重试', async () => {
    vi.useFakeTimers();
    const runner = new CountingRunner(() => ({ exitCode: 1, stdout: '', stderr: '失败' }));
    const provider = new FcdevProjectInfoProvider(runner, fakeLogger());
    await provider.get(file);
    await provider.get(file);
    expect(runner.calls).toBe(1);
    vi.advanceTimersByTime(30_001);
    await provider.get(file);
    expect(runner.calls).toBe(2);
  });
});
