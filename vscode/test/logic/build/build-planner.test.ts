// ------------------------------------------------------------------------
// 名称：build-planner.test.ts
// 说明：测试构建计划：路径规则（当前目录、目录、文件）、源文件收集、目标与选项的报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：每个用例使用独立的临时项目，只规划不运行工具。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { collectSources } from '../../../src/logic/build/build-planner';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

/** 最小可链接的 NES C 程序。 */
const NES_MAIN = 'int main(void) { for (;;) { } }\n';

describe('构建计划', () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', name: 'game' }));
    writeTestFile(temp.dir, 'src/main.c', NES_MAIN);
    writeTestFile(temp.dir, 'src/video.s', '');
  });

  afterEach(() => temp.cleanup());

  it('没有路径参数时以终端当前目录为一个项目', () => {
    const plan = service.plan({ cwd: temp.dir });
    expect(plan.mode).toBe('directory');
    expect(plan.sources).toHaveLength(2);
    expect(plan.programName).toBe('game');
  });

  it('目录参数把整个目录作为一个程序，程序名取配置里的 name', () => {
    const plan = service.plan({ pathArg: 'src', cwd: temp.dir });
    expect(plan.mode).toBe('directory');
    expect(plan.programName).toBe('game');
    expect(plan.outputFile).toBe(path.join(temp.dir, 'build', 'nes', 'game.nes'));
  });

  it('没有配置 name 时程序名取目录名', () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    expect(service.plan({ pathArg: 'src', cwd: temp.dir }).programName).toBe('src');
  });

  it('文件参数只构建这一个文件，程序名取文件名', () => {
    const plan = service.plan({ pathArg: 'src/main.c', cwd: temp.dir });
    expect(plan.mode).toBe('file');
    expect(plan.sources).toEqual([path.join(temp.dir, 'src', 'main.c')]);
    expect(plan.programName).toBe('main');
  });

  it('路径不存在、不是源文件时报错', () => {
    expect(() => service.plan({ pathArg: 'nope', cwd: temp.dir })).toThrow(expect.objectContaining({ code: 'PATH_NOT_FOUND' }));
    writeTestFile(temp.dir, 'src/readme.md', '');
    expect(() => service.plan({ pathArg: 'src/readme.md', cwd: temp.dir })).toThrow(expect.objectContaining({ code: 'NOT_A_SOURCE' }));
  });

  it('找不到配置且没有指定目标时报错，指定目标可覆盖配置', () => {
    const other = createTempDir();
    try {
      writeTestFile(other.dir, 'a.c', NES_MAIN);
      expect(() => service.plan({ pathArg: 'a.c', cwd: other.dir })).toThrow(expect.objectContaining({ code: 'TARGET_MISSING' }));
      expect(service.plan({ pathArg: 'a.c', cwd: other.dir, targetOverride: 'nes' }).target.name).toBe('nes');
    } finally {
      other.cleanup();
    }
  });

  it('不存在的目标与不存在的目标选项报错', () => {
    expect(() => service.plan({ cwd: temp.dir, targetOverride: 'nope' })).toThrow(expect.objectContaining({ code: 'TARGET_UNKNOWN' }));
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', targetOptions: { color: true } }));
    expect(() => service.plan({ cwd: temp.dir })).toThrow(expect.objectContaining({ code: 'TARGET_OPTION_UNKNOWN' }));
  });

  it('学习机的程序名不符合 8.3 时报错', () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'toolongname' }));
    expect(() => service.plan({ pathArg: 'src', cwd: temp.dir })).toThrow(expect.objectContaining({ code: 'NAME_INVALID' }));
  });
});

describe('源文件收集', () => {
  const fileSystem = new NodeFileSystem();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('递归收集，并跳过 build、node_modules、点目录与指定的输出目录', () => {
    for (const file of ['main.c', 'sub/util.s', 'sub/readme.md', 'build/old.c', 'node_modules/x/y.c', '.cache/z.c', 'dist/out.c']) {
      writeTestFile(temp.dir, file, '');
    }
    const found = collectSources(fileSystem, temp.dir, [path.join(temp.dir, 'dist')]).map((file) => path.relative(temp.dir, file).split(path.sep).join('/'));
    expect(found).toEqual(['main.c', 'sub/util.s']);
  });

  it('目录里没有源文件时报错', () => {
    writeTestFile(temp.dir, 'readme.md', '');
    expect(() => collectSources(fileSystem, temp.dir)).toThrow(expect.objectContaining({ code: 'NO_SOURCES' }));
  });
});
