// ------------------------------------------------------------------------
// 名称：fcdev-cli-runner.test.ts
// 说明：测试命令行运行器：向上查找 node_modules/fcdev、传参与工作目录、退出码与输出、没安装时报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：在临时目录里写一个假的 fcdev 脚本，用真实的 node 进程运行；不依赖本机是否装了 fcdev。
// ------------------------------------------------------------------------

import { mkdirSync, mkdtempSync, realpathSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FcdevCliRunner, locateFcdevScript } from '../../src/infra/project/fcdev-cli-runner';
import { UserFacingError } from '../../src/shared/errors/user-facing-error';

/** 假 fcdev：把参数与工作目录打印成 JSON；第一个参数是 fail 时写标准错误并以 3 退出。 */
const FAKE_FCDEV = [
  "const args = process.argv.slice(2);",
  "if (args[0] === 'fail') { console.error('出错了'); process.exit(3); }",
  'console.log(JSON.stringify({ args, cwd: process.cwd() }));'
].join('\n');

let temp = '';

beforeEach(() => {
  temp = realpathSync(mkdtempSync(path.join(tmpdir(), 'fcdev-ext-')));
});

afterEach(() => {
  rmSync(temp, { recursive: true, force: true });
});

/** 在目录下安装假 fcdev。 */
function installFakeFcdev(directory: string): void {
  const bin = path.join(directory, 'node_modules', 'fcdev', 'bin');
  mkdirSync(bin, { recursive: true });
  writeFileSync(path.join(bin, 'fcdev.mjs'), FAKE_FCDEV);
}

describe('查找脚本', () => {
  it('从项目目录向上查找，找不到时为空', () => {
    installFakeFcdev(temp);
    const nested = path.join(temp, 'packages', 'game');
    mkdirSync(nested, { recursive: true });
    expect(locateFcdevScript(nested)).toBe(path.join(temp, 'node_modules', 'fcdev', 'bin', 'fcdev.mjs'));
    expect(locateFcdevScript(path.join(tmpdir(), 'fcdev-ext-never-installed-here'))).toBeUndefined();
  });
});

describe('运行命令', () => {
  it('在项目目录里运行，参数原样传入，返回标准输出', async () => {
    installFakeFcdev(temp);
    const result = await new FcdevCliRunner().run(temp, ['info', '--json']);
    expect(result.exitCode).toBe(0);
    expect(JSON.parse(result.stdout)).toEqual({ args: ['info', '--json'], cwd: temp });
  });

  it('非零退出码不抛错，带出标准错误', async () => {
    installFakeFcdev(temp);
    const result = await new FcdevCliRunner().run(temp, ['fail']);
    expect(result.exitCode).toBe(3);
    expect(result.stderr).toContain('出错了');
  });

  it('项目里没有安装 fcdev 时抛出可预期的错误，提示先 npm install', async () => {
    const error = await new FcdevCliRunner().run(temp, ['sync']).catch((caught: unknown) => caught);
    expect(error).toBeInstanceOf(UserFacingError);
    expect((error as Error).message).toContain('npm install');
  });
});
