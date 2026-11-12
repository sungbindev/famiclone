// ------------------------------------------------------------------------
// 名称：cli.test.mjs
// 说明：测试命令行：帮助、版本、错误输出与退出码、build、info、sync。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：不启动子进程，直接调用 runCli，输出流用内存数组代替。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { runCli } from '../src/cli/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

describe('命令行', () => {
  let temp;
  let stdout;
  let stderr;

  /** 运行命令行并返回退出码 */
  function run(argv) {
    return runCli(argv, {
      cwd: temp.dir,
      env: process.env,
      stdout: { write: (text) => stdout.push(text) },
      stderr: { write: (text) => stderr.push(text) },
    });
  }

  beforeEach(() => {
    temp = createTempDir();
    stdout = [];
    stderr = [];
  });

  afterEach(() => temp.cleanup());

  it('--help 与不带命令都显示帮助，退出码 0', async () => {
    assert.equal(await run(['--help']), 0);
    assert.match(stdout.join(''), /用法：fcdev/);
    stdout.length = 0;
    assert.equal(await run([]), 0);
    assert.match(stdout.join(''), /build/);
  });

  it('--version 输出版本号', async () => {
    assert.equal(await run(['--version']), 0);
    assert.match(stdout.join(''), /^\d+\.\d+\.\d+\n$/);
  });

  it('未知命令返回 1，并提示怎么查看可用命令', async () => {
    assert.equal(await run(['nope']), 1);
    assert.match(stderr.join(''), /未知的命令：nope/);
    assert.match(stderr.join(''), /--help/);
  });

  it('没有指定目标时报错并提示怎么指定', async () => {
    writeTestFile(temp.dir, 'a.c', 'int main(void) { return 0; }\n');
    assert.equal(await run(['build', 'a.c']), 1);
    assert.match(stderr.join(''), /没有指定目标/);
    assert.match(stderr.join(''), /--target/);
  });

  it('build 多个路径报错', async () => {
    assert.equal(await run(['build', 'a.c', 'b.c']), 1);
    assert.match(stderr.join(''), /只接受一个路径参数/);
  });

  it('run 多个路径报错', async () => {
    assert.equal(await run(['run', 'a.c', 'b.c']), 1);
    assert.match(stderr.join(''), /run 只接受一个路径参数/);
  });

  it('debug 没有给命令时提示用法', async () => {
    assert.equal(await run(['debug']), 1);
    assert.match(stderr.join(''), /debug 需要一条调试命令/);
  });
});

describe('命令行（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;
  let stdout;
  let stderr;

  /** 运行命令行并返回退出码 */
  function run(argv) {
    return runCli(argv, {
      cwd: temp.dir,
      env: process.env,
      stdout: { write: (text) => stdout.push(text) },
      stderr: { write: (text) => stderr.push(text) },
    });
  }

  beforeEach(() => {
    temp = createTempDir();
    stdout = [];
    stderr = [];
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'src/hello.c', 'int main(void) { for (;;) { } }\n');
  });

  afterEach(() => temp.cleanup());

  it('build 成功后输出产物路径与大小', async () => {
    assert.equal(await run(['build', 'src/hello.c']), 0);
    assert.match(stdout.join(''), /已生成 build[\\/]nes[\\/]hello\.nes（\d+ 字节/);
    assert.ok(existsSync(path.join(temp.dir, 'build', 'nes', 'hello.nes')));
  });

  it('info --json 输出目标、工具链与头文件目录', async () => {
    assert.equal(await run(['info', '--json']), 0);
    const info = JSON.parse(stdout.join(''));
    assert.equal(info.target.name, 'nes');
    assert.ok(info.includeDirs.length >= 2);
    assert.equal(info.defines.__NES__, 1);
    // 编辑器兼容宏：调用约定关键字定义成空
    assert.equal(info.defines.__fastcall__, '');
    assert.equal(info.defines.__cdecl__, '');
  });

  it('sync 生成 c_cpp_properties.json 与 compile_flags.txt，重复运行只更新 fcdev 的配置', async () => {
    writeTestFile(temp.dir, '.vscode/c_cpp_properties.json', JSON.stringify({ version: 4, configurations: [{ name: 'mine', includePath: [] }] }));
    assert.equal(await run(['sync']), 0);
    assert.equal(await run(['sync']), 0);
    const document = JSON.parse(readFileSync(path.join(temp.dir, '.vscode', 'c_cpp_properties.json'), 'utf8'));
    assert.deepEqual(document.configurations.map((item) => item.name), ['mine', 'fcdev']);
    const flags = readFileSync(path.join(temp.dir, 'compile_flags.txt'), 'utf8').split('\n');
    assert.equal(flags[0], '-DFCDEV_GENERATED');
    assert.ok(flags.some((line) => line.startsWith('-I') && line.includes('fcnes') === false));
    assert.ok(flags.includes('-D__fastcall__='), 'compile_flags.txt 里要有将 __fastcall__ 定义成空的宏');
    const fcdev = document.configurations.find((item) => item.name === 'fcdev');
    assert.ok(fcdev.defines.includes('__fastcall__='));
  });

  it('sync 不覆盖用户自己的 compile_flags.txt', async () => {
    writeTestFile(temp.dir, 'compile_flags.txt', '-Wall\n');
    assert.equal(await run(['sync']), 0);
    assert.equal(readFileSync(path.join(temp.dir, 'compile_flags.txt'), 'utf8'), '-Wall\n');
    assert.match(stderr.join(''), /没有覆盖/);
  });
});
