// ------------------------------------------------------------------------
// 名称：build-service-comparer.test.ts
// 说明：测试构建比较：构建项目目录或单个文件，产物与反汇编工程的原映像逐字节比较，
//       读不到工程或映像时报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：构建服务用替身（产物由替身写出），文件在临时目录里真实读写。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { BuildInput, BuildOutcome, BuildService } from '../../../src/contracts/build/build-service';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { BuildServiceComparer } from '../../../src/logic/disassembly/build-service-comparer';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

describe('构建并比较', () => {
  let temp: TempDir;
  let projectDir: string;
  let buildDir: string;
  let built: Uint8Array;
  let inputs: BuildInput[];

  /** 替身：记录输入，把预先设定的产物写到 build/out.bin。 */
  function createService(): BuildService {
    return {
      plan: vi.fn(),
      buildDisk: vi.fn(),
      build: async (input: BuildInput): Promise<BuildOutcome> => {
        inputs.push(input);
        const outputFile = writeTestFile(buildDir, 'build/out.bin', built);
        return { plan: {} as BuildOutcome['plan'], result: { programName: 'out', outputFile, size: built.length, compiled: 1, reused: 0 } };
      }
    };
  }

  beforeEach(() => {
    temp = createTempDir();
    projectDir = path.join(temp.dir, 'proj');
    buildDir = path.join(temp.dir, 'port');
    inputs = [];
    built = Uint8Array.from([1, 2, 3, 4]);
    writeTestFile(projectDir, 'disasm.json', JSON.stringify({ image: { path: '../orig.bin' } }));
    writeTestFile(temp.dir, 'orig.bin', Uint8Array.from([1, 2, 3, 4]));
    writeTestFile(buildDir, 'main.s', '');
  });

  afterEach(() => temp.cleanup());

  it('构建目录：产物与映像一致', async () => {
    const result = await new BuildServiceComparer(new NodeFileSystem(), createService()).buildAndCompare(projectDir, buildDir);
    expect(result).toMatchObject({ identical: true, firstDifference: null, builtSize: 4, imageSize: 4, builtFile: path.join(buildDir, 'build', 'out.bin') });
    expect(inputs).toEqual([{ cwd: buildDir }]);
  });

  it('构建单个文件：按文件名与所在目录构建，并给出第一处不同', async () => {
    built = Uint8Array.from([1, 2, 9, 4]);
    const result = await new BuildServiceComparer(new NodeFileSystem(), createService()).buildAndCompare(projectDir, path.join(buildDir, 'main.s'));
    expect(result).toMatchObject({ identical: false, firstDifference: 2 });
    expect(inputs).toEqual([{ pathArg: 'main.s', cwd: buildDir }]);
  });

  it('长度不同时较短的一方结束的位置算不同', async () => {
    built = Uint8Array.from([1, 2, 3]);
    const result = await new BuildServiceComparer(new NodeFileSystem(), createService()).buildAndCompare(projectDir, buildDir);
    expect(result).toMatchObject({ identical: false, firstDifference: 3, builtSize: 3, imageSize: 4 });
  });

  it('读不到工程、工程里没有映像路径、读不到映像、找不到构建目标时报错', async () => {
    const comparer = new BuildServiceComparer(new NodeFileSystem(), createService());
    await expect(comparer.buildAndCompare(path.join(temp.dir, 'none'), buildDir)).rejects.toThrow(/读不到工程文件/);
    writeTestFile(temp.dir, 'bad/disasm.json', '{}');
    await expect(comparer.buildAndCompare(path.join(temp.dir, 'bad'), buildDir)).rejects.toThrow(/没有映像路径/);
    writeTestFile(temp.dir, 'lost/disasm.json', JSON.stringify({ image: { path: 'gone.bin' } }));
    await expect(comparer.buildAndCompare(path.join(temp.dir, 'lost'), buildDir)).rejects.toThrow(/读不到映像文件/);
    await expect(comparer.buildAndCompare(projectDir, path.join(temp.dir, 'nothing'))).rejects.toThrow(/找不到要构建的/);
  });
});
