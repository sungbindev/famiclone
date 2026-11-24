// ------------------------------------------------------------------------
// 名称：default-project-describer.test.ts
// 说明：测试项目描述与项目信息提供者：目标与工具链目录的汇总、预定义宏、
//       找项目、缓存、失效、失败后的重试与日志。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：目标用扩展自带的，工具链用临时目录里只含空文件的假目录（不运行工具）；
//       提供者用替身描述器，用假定时器推进重试间隔。
// ------------------------------------------------------------------------

import { mkdirSync, realpathSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { ProjectDescriber, ProjectDescription } from '../../../src/contracts/project/project-describer';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { BundledToolchainLocator } from '../../../src/infra/toolchain/bundled-toolchain-locator';
import { DefaultProjectDescriber } from '../../../src/logic/project/default-project-describer';
import { DescribingProjectInfoProvider } from '../../../src/logic/project/describing-project-info-provider';
import { FileProjectConfigReader } from '../../../src/logic/project/file-project-config-reader';
import { FileTargetRepository } from '../../../src/logic/targets/file-target-repository';
import type { Logger } from '../../../src/platform/logging/logger';
import { UserFacingError } from '../../../src/shared/errors/user-facing-error';
import { formatProjectDescription } from '../../../src/shared/project/project-info';
import { createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

const { fileSystem, resources } = createRealBuildEnvironment();

describe('项目描述', () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  /** 目标用扩展自带的，工具链用临时目录里只含空文件的假目录（不运行工具）。 */
  function createDescriber(): DefaultProjectDescriber {
    for (const tool of ['cc65', 'ca65', 'ld65']) writeTestFile(temp.dir, `toolchain/bin/${tool}.exe`, '');
    const toolchainDirectory = path.join(temp.dir, 'toolchain');
    return new DefaultProjectDescriber(
      new FileProjectConfigReader(fileSystem),
      new FileTargetRepository(fileSystem, resources),
      new BundledToolchainLocator(fileSystem, { ...resources, toolchainDirectory })
    );
  }

  it('包含目标、头文件目录、汇编引入目录、预定义宏与输出目录', () => {
    writeTestFile(temp.dir, 'project/fcdev.json', JSON.stringify({ target: 'nes', include: ['inc'], define: { DEBUG: 1 } }));
    const project = path.join(temp.dir, 'project');
    const toolchain = path.join(temp.dir, 'toolchain');
    const description = createDescriber().describe({ cwd: project });
    expect(description.targetName).toBe('nes');
    expect(description.root).toBe(project);
    expect(description.includeDirs).toEqual([
      path.join(resources.targetsDirectory, 'nes', 'include'),
      path.join(project, 'inc'),
      path.join(toolchain, 'include')
    ]);
    expect(description.asmIncludeDirs[0]).toBe(path.join(resources.targetsDirectory, 'nes', 'asminc'));
    expect(description.defines).toMatchObject({ __fastcall__: '', __cdecl__: '', __CC65__: 1, __NES__: 1, DEBUG: 1 });
    expect(description.outDir).toBe(path.join(project, 'build'));
    expect(formatProjectDescription(description)).toMatch(/^项目根目录：.*\n配置文件：.*fcdev\.json\n目标：nes（任天堂红白机（NES））/);
  });

  it('没有指定目标时报错，指定的目标可以覆盖配置', () => {
    writeTestFile(temp.dir, 'project/fcdev.json', '{}');
    const describer = createDescriber();
    const project = path.join(temp.dir, 'project');
    expect(() => describer.describe({ cwd: project })).toThrow(expect.objectContaining({ code: 'TARGET_MISSING' }));
    expect(describer.describe({ cwd: project, target: 'sb2000' }).targetName).toBe('sb2000');
  });
});

/** 一份假的项目描述。 */
function fakeDescription(root: string): ProjectDescription {
  return {
    root,
    configFile: path.join(root, 'fcdev.json'),
    targetName: 'nes',
    targetTitle: 'NES',
    toolchainHome: 'tc',
    includeDirs: ['inc'],
    asmIncludeDirs: ['asminc'],
    defines: {},
    outDir: path.join(root, 'build')
  };
}

/** 记录调用次数的描述器替身。 */
class CountingDescriber implements ProjectDescriber {
  calls = 0;

  /** @param next 每次调用的结果或错误。 */
  constructor(private readonly next: (root: string) => ProjectDescription | Error) {}

  /** @inheritdoc */
  describe(input: { cwd: string }): ProjectDescription {
    this.calls += 1;
    const outcome = this.next(input.cwd);
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

describe('项目信息提供者', () => {
  let temp = '';
  let file = '';

  beforeEach(() => {
    const created = createTempDir();
    temp = realpathSync(created.dir);
    mkdirSync(path.join(temp, 'src'));
    writeTestFile(temp, 'fcdev.json', '{}');
    file = path.join(temp, 'src', 'main.s');
    cleanups.push(created.cleanup);
  });

  const cleanups: Array<() => void> = [];

  afterEach(() => {
    vi.useRealTimers();
    for (const cleanup of cleanups.splice(0)) cleanup();
  });

  it('从文件所在目录向上找 fcdev.json，没有时为空', () => {
    const provider = new DescribingProjectInfoProvider(new NodeFileSystem(), new CountingDescriber(fakeDescription), fakeLogger());
    expect(provider.findProjectRoot(file)).toBe(temp);
    expect(provider.findProjectRoot(path.join(tmpdir(), 'fcdev-ext-no-project', 'a.s'))).toBeUndefined();
  });

  it('成功时返回项目信息，同一项目只读取一次', async () => {
    const describer = new CountingDescriber(fakeDescription);
    const provider = new DescribingProjectInfoProvider(new NodeFileSystem(), describer, fakeLogger());
    expect(await provider.get(file)).toEqual({ root: temp, targetName: 'nes', includeDirs: ['inc'], asmIncludeDirs: ['asminc'] });
    await provider.get(path.join(temp, 'other.s'));
    expect(describer.calls).toBe(1);
  });

  it('文件不在 FC Dev 项目里时不读取；失效后重新读取', async () => {
    const describer = new CountingDescriber(fakeDescription);
    const provider = new DescribingProjectInfoProvider(new NodeFileSystem(), describer, fakeLogger());
    expect(await provider.get(path.join(tmpdir(), 'fcdev-ext-no-project', 'a.s'))).toBeUndefined();
    expect(describer.calls).toBe(0);
    await provider.get(file);
    provider.invalidate(temp);
    await provider.get(file);
    provider.invalidate();
    await provider.get(file);
    expect(describer.calls).toBe(3);
  });

  it('可预期的错误记日志，未预期的错误记为错误，都返回空', async () => {
    const outcomes: Array<ProjectDescription | Error> = [new UserFacingError('找不到目标'), new Error('代码缺陷')];
    const logger = fakeLogger();
    const provider = new DescribingProjectInfoProvider(new NodeFileSystem(), new CountingDescriber(() => outcomes.shift() ?? new Error('多余的调用')), logger);
    for (let attempt = 0; attempt < 2; attempt += 1) {
      provider.invalidate();
      expect(await provider.get(file)).toBeUndefined();
    }
    expect(logger.infos).toHaveLength(1);
    expect(logger.infos[0]).toContain('找不到目标');
    expect(logger.errors).toHaveLength(1);
  });

  it('失败的结果在 30 秒内沿用，之后允许重试', async () => {
    vi.useFakeTimers();
    const describer = new CountingDescriber(() => new UserFacingError('失败'));
    const provider = new DescribingProjectInfoProvider(new NodeFileSystem(), describer, fakeLogger());
    await provider.get(file);
    await provider.get(file);
    expect(describer.calls).toBe(1);
    vi.advanceTimersByTime(30_001);
    await provider.get(file);
    expect(describer.calls).toBe(2);
  });
});
