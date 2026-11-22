// ------------------------------------------------------------------------
// 名称：file-intellisense-sync-service.test.ts
// 说明：测试 C 智能感知配置同步：写 c_cpp_properties.json 与 compile_flags.txt、内容没变不写盘、
//       保留用户自己的文件、失败转成结果、同一项目依次同步、同步整个工作区。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：描述器用替身；文件在临时目录里真实读写。
// ------------------------------------------------------------------------

import { readFileSync, statSync, utimesSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import type { ProjectDescriber, ProjectDescription } from '../../../src/contracts/project/project-describer';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { FileIntelliSenseSyncService } from '../../../src/logic/c-intellisense/file-intellisense-sync-service';
import { UserFacingError } from '../../../src/shared/errors/user-facing-error';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

/** 生成描述；include 目录可以换，模拟扩展升级后路径变了。 */
function descriptionFor(root: string, includeDir: string): ProjectDescription {
  return {
    root,
    configFile: path.join(root, 'fcdev.json'),
    targetName: 'nes',
    targetTitle: 'NES',
    toolchainHome: 'tc',
    includeDirs: [includeDir],
    asmIncludeDirs: [],
    defines: { __NES__: 1 },
    outDir: path.join(root, 'build')
  };
}

describe('C 智能感知配置同步', () => {
  let temp: TempDir;
  let includeDir = 'ext-1/include';
  let failure: Error | undefined;
  const describer: ProjectDescriber = {
    describe: ({ cwd }) => {
      if (failure !== undefined) throw failure;
      return descriptionFor(cwd, includeDir);
    }
  };

  beforeEach(() => {
    temp = createTempDir();
    includeDir = 'ext-1/include';
    failure = undefined;
  });

  afterEach(() => temp.cleanup());

  /** 创建服务；locator 返回给定的项目根目录。 */
  function createService(...roots: string[]): FileIntelliSenseSyncService {
    return new FileIntelliSenseSyncService(new NodeFileSystem(), describer, { findProjectRoots: async () => roots });
  }

  it('生成 c_cpp_properties.json 与 compile_flags.txt', async () => {
    const result = await createService().syncProject(temp.dir);
    expect(result.ok).toBe(true);
    expect(result.message).toContain('c_cpp_properties.json');
    const properties = JSON.parse(readFileSync(path.join(temp.dir, '.vscode', 'c_cpp_properties.json'), 'utf8')) as { configurations: Array<{ includePath: string[] }> };
    expect(properties.configurations[0]?.includePath).toEqual(['ext-1/include']);
    expect(readFileSync(path.join(temp.dir, 'compile_flags.txt'), 'utf8')).toMatch(/^-DFCDEV_GENERATED\n-Iext-1\/include/);
  });

  it('内容没变时不写盘；路径变了（扩展升级）时改写', async () => {
    const service = createService();
    await service.syncProject(temp.dir);
    const file = path.join(temp.dir, '.vscode', 'c_cpp_properties.json');
    const old = new Date(2020, 0, 1);
    utimesSync(file, old, old);
    await service.syncProject(temp.dir);
    expect(statSync(file).mtime.getFullYear()).toBe(2020);
    includeDir = 'ext-2/include';
    await service.syncProject(temp.dir);
    expect(readFileSync(file, 'utf8')).toContain('ext-2/include');
    expect(readFileSync(path.join(temp.dir, 'compile_flags.txt'), 'utf8')).toContain('-Iext-2/include');
  });

  it('用户自己的 compile_flags.txt 不覆盖，并在说明里提到', async () => {
    writeTestFile(temp.dir, 'compile_flags.txt', '-Imine\n');
    const result = await createService().syncProject(temp.dir);
    expect(result.ok).toBe(true);
    expect(result.message).toContain('没有覆盖');
    expect(readFileSync(path.join(temp.dir, 'compile_flags.txt'), 'utf8')).toBe('-Imine\n');
  });

  it('已有的 c_cpp_properties.json 里其他配置保留', async () => {
    writeTestFile(temp.dir, '.vscode/c_cpp_properties.json', JSON.stringify({ version: 4, configurations: [{ name: 'Win32' }] }));
    await createService().syncProject(temp.dir);
    const properties = JSON.parse(readFileSync(path.join(temp.dir, '.vscode', 'c_cpp_properties.json'), 'utf8')) as { configurations: Array<{ name: string }> };
    expect(properties.configurations.map((item) => item.name)).toEqual(['Win32', 'fcdev']);
  });

  it('可预期的错误转成失败结果，其他异常原样抛出', async () => {
    failure = new UserFacingError('没有指定目标');
    expect(await createService().syncProject(temp.dir)).toEqual({ root: temp.dir, ok: false, message: '没有指定目标' });
    failure = new Error('代码缺陷');
    await expect(createService().syncProject(temp.dir)).rejects.toThrow('代码缺陷');
  });

  it('配置文件损坏时报失败，不改写它', async () => {
    writeFileSync(path.join(temp.dir, 'bad.json'), '');
    writeTestFile(temp.dir, '.vscode/c_cpp_properties.json', '{');
    const result = await createService().syncProject(temp.dir);
    expect(result.ok).toBe(false);
    expect(readFileSync(path.join(temp.dir, '.vscode', 'c_cpp_properties.json'), 'utf8')).toBe('{');
  });

  it('同步工作区里的全部项目；没有项目时为空', async () => {
    const other = createTempDir();
    try {
      const results = await createService(temp.dir, other.dir).syncWorkspace();
      expect(results.map((item) => item.root)).toEqual([temp.dir, other.dir]);
      expect(await createService().syncWorkspace()).toEqual([]);
    } finally {
      other.cleanup();
    }
  });
});
