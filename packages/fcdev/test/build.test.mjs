// ------------------------------------------------------------------------
// 名称：build.test.mjs
// 说明：测试构建：构建计划的路径规则（当前目录、目录、文件），以及用真实 cc65 构建 NES 程序（含增量、错误提示）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：集成部分在本机没有 cc65 时跳过；每个用例使用独立的临时项目。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, readFileSync, utimesSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath, createBuildPlan } from '../src/build/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 最小可链接的 NES C 程序 */
const NES_MAIN = 'int main(void) { for (;;) { } }\n';

describe('构建计划', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', name: 'game' }));
    writeTestFile(temp.dir, 'src/main.c', NES_MAIN);
    writeTestFile(temp.dir, 'src/video.s', '');
  });

  afterEach(() => temp.cleanup());

  it('没有路径参数时以终端当前目录为一个项目', () => {
    const plan = createBuildPlan({ cwd: temp.dir });
    assert.equal(plan.mode, 'directory');
    assert.equal(plan.sources.length, 2);
    assert.equal(plan.programName, 'game');
  });

  it('目录参数把整个目录作为一个程序，程序名取配置里的 name', () => {
    const plan = createBuildPlan({ pathArg: 'src', cwd: temp.dir });
    assert.equal(plan.mode, 'directory');
    assert.equal(plan.programName, 'game');
    assert.equal(plan.outputFile, path.join(temp.dir, 'build', 'nes', 'game.nes'));
  });

  it('没有配置 name 时程序名取目录名', () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    assert.equal(createBuildPlan({ pathArg: 'src', cwd: temp.dir }).programName, 'src');
  });

  it('文件参数只构建这一个文件，程序名取文件名', () => {
    const plan = createBuildPlan({ pathArg: 'src/main.c', cwd: temp.dir });
    assert.equal(plan.mode, 'file');
    assert.deepEqual(plan.sources, [path.join(temp.dir, 'src', 'main.c')]);
    assert.equal(plan.programName, 'main');
  });

  it('路径不存在、不是源文件时报错', () => {
    assert.throws(() => createBuildPlan({ pathArg: 'nope', cwd: temp.dir }), (error) => error.code === 'PATH_NOT_FOUND');
    writeTestFile(temp.dir, 'src/readme.md', '');
    assert.throws(() => createBuildPlan({ pathArg: 'src/readme.md', cwd: temp.dir }), (error) => error.code === 'NOT_A_SOURCE');
  });

  it('找不到配置且没有 --target 时报错，--target 可覆盖配置', () => {
    const other = createTempDir();
    try {
      writeTestFile(other.dir, 'a.c', NES_MAIN);
      assert.throws(() => createBuildPlan({ pathArg: 'a.c', cwd: other.dir }), (error) => error.code === 'TARGET_MISSING');
      assert.equal(createBuildPlan({ pathArg: 'a.c', cwd: other.dir, targetOverride: 'nes' }).target.name, 'nes');
    } finally {
      other.cleanup();
    }
  });

  it('不存在的目标与不存在的目标选项报错', () => {
    assert.throws(() => createBuildPlan({ cwd: temp.dir, targetOverride: 'nope' }), (error) => error.code === 'TARGET_UNKNOWN');
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', targetOptions: { color: true } }));
    assert.throws(() => createBuildPlan({ cwd: temp.dir }), (error) => error.code === 'TARGET_OPTION_UNKNOWN');
  });
});

describe('用真实 cc65 构建', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
  });

  afterEach(() => temp.cleanup());

  /** 构建并返回结果 */
  async function build(pathArg) {
    const output = [];
    const reporter = { log() {}, write: (text) => output.push(text) };
    const { result } = await buildPath({ pathArg, cwd: temp.dir }, reporter);
    return { result, output: output.join('') };
  }

  it('C 程序构建出 iNES 文件', async () => {
    writeTestFile(temp.dir, 'src/hello.c', NES_MAIN);
    const { result } = await build('src/hello.c');
    const image = readFileSync(result.outputFile);
    assert.equal(image.subarray(0, 4).toString('latin1'), 'NES\x1a');
    assert.equal(result.compiled, 1);
  });

  it('汇编程序构建成功（导出 _main）', async () => {
    writeTestFile(temp.dir, 'src/hello.s', '.export _main\n.segment "CODE"\n_main:\n@loop: jmp @loop\n');
    const { result } = await build('src/hello.s');
    assert.ok(existsSync(result.outputFile));
  });

  it('目录构建把 C 与汇编文件链接成一个程序', async () => {
    writeTestFile(temp.dir, 'game/main.c', 'extern void spin(void);\nint main(void) { spin(); return 0; }\n');
    writeTestFile(temp.dir, 'game/spin.s', '.export _spin\n.segment "CODE"\n_spin:\n@loop: jmp @loop\n');
    const { result } = await build('game');
    assert.equal(path.basename(result.outputFile), 'game.nes');
    assert.equal(result.compiled, 2);
  });

  it('第二次构建沿用未改动的文件，改动源文件后只重编它', async () => {
    const main = writeTestFile(temp.dir, 'game/main.c', 'extern void spin(void);\nint main(void) { spin(); return 0; }\n');
    writeTestFile(temp.dir, 'game/spin.s', '.export _spin\n.segment "CODE"\n_spin:\n@loop: jmp @loop\n');
    await build('game');
    const second = await build('game');
    assert.equal(second.result.compiled, 0);
    assert.equal(second.result.reused, 2);
    const future = new Date(Date.now() + 5000);
    utimesSync(main, future, future);
    const third = await build('game');
    assert.equal(third.result.compiled, 1);
    assert.equal(third.result.reused, 1);
  });

  it('改动被包含的头文件后重编依赖它的 C 文件', async () => {
    const header = writeTestFile(temp.dir, 'game/value.h', '#define VALUE 1\n');
    writeTestFile(temp.dir, 'game/main.c', '#include "value.h"\nint main(void) { return VALUE; }\n');
    await build('game');
    const future = new Date(Date.now() + 5000);
    utimesSync(header, future, future);
    assert.equal((await build('game')).result.compiled, 1);
  });

  it('改动目标选项后全部重编', async () => {
    writeTestFile(temp.dir, 'src/hello.c', NES_MAIN);
    await build('src/hello.c');
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', targetOptions: { font: false } }));
    assert.equal((await build('src/hello.c')).result.compiled, 1);
  });

  it('C 语法错误时失败，工具输出原样透传', async () => {
    writeTestFile(temp.dir, 'src/bad.c', 'int main(void) { return x; }\n');
    const output = [];
    const reporter = { log() {}, write: (text) => output.push(text) };
    await assert.rejects(buildPath({ pathArg: 'src/bad.c', cwd: temp.dir }, reporter), (error) => error.code === 'TOOL_FAILED');
    assert.match(output.join(''), /bad\.c\(1\): Error: Undefined symbol: 'x'|bad\.c:1: Error/);
  });

  it('缺少 main 时给出提示', async () => {
    writeTestFile(temp.dir, 'src/nomain.c', 'int value;\n');
    await assert.rejects(buildPath({ pathArg: 'src/nomain.c', cwd: temp.dir }), (error) => error.code === 'TOOL_FAILED' && error.hint.includes('没有找到 main'));
  });

  it('目录里有两个 main 时给出提示', async () => {
    writeTestFile(temp.dir, 'game/a.c', 'int main(void) { return 0; }\n');
    writeTestFile(temp.dir, 'game/b.c', 'int main(void) { return 1; }\n');
    await assert.rejects(buildPath({ pathArg: 'game', cwd: temp.dir }), (error) => error.code === 'TOOL_FAILED' && error.hint.includes('多个 main'));
  });
});
