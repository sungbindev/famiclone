// ------------------------------------------------------------------------
// 名称：fcdev-build-comparer.test.ts
// 说明：测试用 fcdev build 构建并与原映像比较：命令参数、产物路径的取法、
//       一致与不一致的结果、各种失败。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：命令行用替身，工程文件与映像、产物都写在临时目录里。
// ------------------------------------------------------------------------

import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { FcdevCommandResult, FcdevCommandRunner } from '../../src/contracts/project/fcdev-command-runner';
import { FcdevBuildComparer } from '../../src/infra/disassembly/fcdev-build-comparer';
import { UserFacingError } from '../../src/shared/errors/user-facing-error';

let root: string;
let projectDir: string;
let buildDir: string;

beforeEach(() => {
  root = mkdtempSync(path.join(tmpdir(), 'fcdev-build-'));
  projectDir = path.join(root, 'project');
  buildDir = path.join(root, 'rebuilt');
  mkdirSync(projectDir, { recursive: true });
  mkdirSync(path.join(buildDir, 'build', 'nes'), { recursive: true });
  writeFileSync(path.join(root, 'game.nes'), Buffer.from([1, 2, 3, 4]));
  writeFileSync(path.join(projectDir, 'disasm.json'), JSON.stringify({ image: { path: '../game.nes' } }));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

/** 返回固定结果的命令行替身。 */
function fakeRunner(result: FcdevCommandResult): FcdevCommandRunner & { run: ReturnType<typeof vi.fn> } {
  return { run: vi.fn(async () => result) };
}

const BUILT_OUTPUT = '已生成 build/nes/rebuilt.nes（4 字节，目标 nes，重新编译 2 个文件，沿用 0 个）\n';

describe('构建并比较', () => {
  it('目录构建：在目录里运行 build，产物与原映像一致', async () => {
    writeFileSync(path.join(buildDir, 'build', 'nes', 'rebuilt.nes'), Buffer.from([1, 2, 3, 4]));
    const runner = fakeRunner({ exitCode: 0, stdout: BUILT_OUTPUT, stderr: '' });
    const result = await new FcdevBuildComparer(runner).buildAndCompare(projectDir, buildDir);
    expect(runner.run).toHaveBeenCalledWith(buildDir, ['build']);
    expect(result).toEqual({ builtFile: path.join(buildDir, 'build', 'nes', 'rebuilt.nes'), builtSize: 4, imageSize: 4, identical: true, firstDifference: null });
  });

  it('文件构建：以文件所在目录为当前目录，把文件名交给 build；产物不同时给出第一处差异', async () => {
    writeFileSync(path.join(buildDir, 'main.s'), '');
    writeFileSync(path.join(buildDir, 'build', 'nes', 'rebuilt.nes'), Buffer.from([1, 2, 9, 4]));
    const runner = fakeRunner({ exitCode: 0, stdout: BUILT_OUTPUT, stderr: '' });
    const result = await new FcdevBuildComparer(runner).buildAndCompare(projectDir, path.join(buildDir, 'main.s'));
    expect(runner.run).toHaveBeenCalledWith(buildDir, ['build', 'main.s']);
    expect(result).toMatchObject({ identical: false, firstDifference: 2 });
  });

  it('构建失败、输出里没有产物路径、读不到工程或映像、构建目标不存在，都报用户能看懂的错误', async () => {
    const failed = new FcdevBuildComparer(fakeRunner({ exitCode: 1, stdout: '', stderr: '链接失败' }));
    await expect(failed.buildAndCompare(projectDir, buildDir)).rejects.toThrow(/fcdev build 失败（退出码 1）：\n链接失败/);
    const silent = new FcdevBuildComparer(fakeRunner({ exitCode: 0, stdout: '完成\n', stderr: '' }));
    await expect(silent.buildAndCompare(projectDir, buildDir)).rejects.toThrow(/找不到产物路径/);
    await expect(silent.buildAndCompare(path.join(root, 'nothing'), buildDir)).rejects.toBeInstanceOf(UserFacingError);
    writeFileSync(path.join(projectDir, 'disasm.json'), JSON.stringify({ image: { path: '../gone.nes' } }));
    await expect(silent.buildAndCompare(projectDir, buildDir)).rejects.toThrow(/读不到映像文件/);
    await expect(silent.buildAndCompare(projectDir, path.join(root, 'missing'))).rejects.toBeInstanceOf(UserFacingError);
  });

  it('工程文件不是预期的结构时报错', async () => {
    writeFileSync(path.join(projectDir, 'disasm.json'), '{"image": {}}');
    await expect(new FcdevBuildComparer(fakeRunner({ exitCode: 0, stdout: BUILT_OUTPUT, stderr: '' })).buildAndCompare(projectDir, buildDir)).rejects.toThrow(/没有映像路径/);
  });
});
