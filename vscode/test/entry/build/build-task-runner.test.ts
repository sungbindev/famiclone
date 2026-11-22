// ------------------------------------------------------------------------
// 名称：build-task-runner.test.ts
// 说明：测试 fcdev 任务的执行：构建、生成磁盘镜像的输出与退出码，
//       可预期的错误写成“错误：…”并带建议，未预期的错误原样抛出；
//       任务定义的解析与问题匹配器对工具输出的识别。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：构建服务用替身，不运行工具；问题匹配器的正则取自 package.json。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it, vi } from 'vitest';
import type { BuildOutcome, BuildService } from '../../../src/contracts/build/build-service';
import type { ProgramRunner } from '../../../src/contracts/emulator/emulator-operations';
import { runBuildTask } from '../../../src/entry/build/build-task-runner';
import { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { parseTaskDefinition } from '../../../src/shared/build/task-definition';

const FOLDER = path.resolve('work');

/** 一次成功构建的结果。 */
function outcomeOf(): BuildOutcome {
  return {
    plan: { target: { name: 'sb2000' } } as BuildOutcome['plan'],
    result: { programName: 'hello', outputFile: path.join(FOLDER, 'build', 'sb2000', 'HELLO.EXE'), size: 420, compiled: 1, reused: 2 }
  };
}

/** 构建服务替身：build 的行为由参数决定。 */
function fakeService(build: BuildService['build']): BuildService {
  return { plan: vi.fn(), build, buildDisk: () => ({ imageFile: path.join(FOLDER, 'build', 'sb2000', 'HELLO.img'), size: 1474560 }) };
}

/** 运行器替身：构建任务用不到它。 */
const UNUSED_RUNNER: ProgramRunner = { run: vi.fn() };

describe('执行任务', () => {
  it('构建成功：输出进度与结果行，退出码 0，输入里带路径与目标', async () => {
    const build = vi.fn(async (_input, reporter) => {
      reporter?.log('编译 hello.c');
      reporter?.write('警告文字\n');
      return outcomeOf();
    });
    const output: string[] = [];
    const code = await runBuildTask(parseTaskDefinition({ command: 'build', path: 'src', target: 'sb2000' }), FOLDER, { buildService: fakeService(build), runner: UNUSED_RUNNER }, (text) => output.push(text));
    expect(code).toBe(0);
    expect(output.join('')).toBe(`编译 hello.c\n警告文字\n已生成 ${path.join('build', 'sb2000', 'HELLO.EXE')}（420 字节，目标 sb2000，重新编译 1 个文件，沿用 2 个）\n`);
    expect(build.mock.calls[0]?.[0]).toEqual({ pathArg: 'src', cwd: FOLDER, targetOverride: 'sb2000' });
  });

  it('生成磁盘镜像时再输出镜像行', async () => {
    const output: string[] = [];
    const code = await runBuildTask(parseTaskDefinition({ command: 'disk' }), FOLDER, { buildService: fakeService(async () => outcomeOf()), runner: UNUSED_RUNNER }, (text) => output.push(text));
    expect(code).toBe(0);
    expect(output.join('')).toContain(`已生成 ${path.join('build', 'sb2000', 'HELLO.img')}（1474560 字节，含 HELLO.EXE）`);
  });

  it('可预期的错误：写“错误：…”与建议，退出码 1；未预期的错误原样抛出', async () => {
    const output: string[] = [];
    const failing = fakeService(async () => {
      throw new FcdevError('链接失败：hello', { code: 'TOOL_FAILED', hint: '没有找到 main' });
    });
    expect(await runBuildTask(parseTaskDefinition({ command: 'build' }), FOLDER, { buildService: failing, runner: UNUSED_RUNNER }, (text) => output.push(text))).toBe(1);
    expect(output.join('')).toBe('错误：链接失败：hello\n没有找到 main\n');
    const broken = fakeService(async () => {
      throw new Error('代码缺陷');
    });
    await expect(runBuildTask(parseTaskDefinition({ command: 'build' }), FOLDER, { buildService: broken, runner: UNUSED_RUNNER }, () => undefined)).rejects.toThrow('代码缺陷');
  });

  it('运行：把路径、目标与 debug 交给运行器，输出构建进度与启动结果，退出码 0', async () => {
    const run = vi.fn(async (_input, reporter) => {
      reporter?.log('编译 hello.c');
      return { pid: 4242, description: 'hello.nes', debug: true, port: 5699, firmwareDisk: false };
    });
    const output: string[] = [];
    const code = await runBuildTask(parseTaskDefinition({ command: 'run', path: 'src', target: 'nes', debug: true }), FOLDER, { buildService: fakeService(async () => outcomeOf()), runner: { run } }, (text) => output.push(text));
    expect(code).toBe(0);
    expect(run.mock.calls[0]?.[0]).toEqual({ build: { pathArg: 'src', cwd: FOLDER, targetOverride: 'nes' }, debug: true });
    expect(output.join('')).toContain('编译 hello.c\n已启动 Ember（进程 4242），加载 hello.nes\n调试模式：端口 5699');
  });

  it('运行时没有设置 Ember：写“错误：…”与设置建议，退出码 1', async () => {
    const runner: ProgramRunner = {
      run: async () => {
        throw new FcdevError('还没有设置 Ember 的位置', { code: 'EMULATOR_PATH_NOT_SET', hint: '在设置里填写 fcdev.emulator.path' });
      }
    };
    const output: string[] = [];
    expect(await runBuildTask(parseTaskDefinition({ command: 'run' }), FOLDER, { buildService: fakeService(async () => outcomeOf()), runner }, (text) => output.push(text))).toBe(1);
    expect(output.join('')).toBe('错误：还没有设置 Ember 的位置\n在设置里填写 fcdev.emulator.path\n');
  });
});

describe('任务定义', () => {
  it('命令必须是 build、disk、run 之一，字段类型要对', () => {
    expect(parseTaskDefinition({ command: 'run', debug: true, path: 'src' })).toEqual({ command: 'run', debug: true, path: 'src', target: undefined });
    expect(() => parseTaskDefinition({ command: 'clean' })).toThrow(expect.objectContaining({ code: 'TASK_INVALID' }));
    expect(() => parseTaskDefinition({ command: 'build', path: 1 })).toThrow(/path/);
    expect(() => parseTaskDefinition({ command: 'build', debug: 'yes' })).toThrow(/debug/);
    expect(() => parseTaskDefinition(null)).toThrow(/对象/);
  });
});

describe('问题匹配器 $fcdev', () => {
  const manifest = JSON.parse(readFileSync(path.join(__dirname, '..', '..', '..', 'package.json'), 'utf8')) as {
    contributes: { problemMatchers: Array<{ name: string; pattern: { regexp: string; file: number; line: number; severity: number; message: number } }> };
  };
  const matcher = manifest.contributes.problemMatchers.find((item) => item.name === 'fcdev');

  /** 用匹配器的正则解析一行工具输出。 */
  function parse(line: string): { file?: string; line?: string; severity?: string; message?: string } | undefined {
    const match = new RegExp(matcher?.pattern.regexp ?? '').exec(line);
    if (match === null || matcher === undefined) return undefined;
    return { file: match[matcher.pattern.file], line: match[matcher.pattern.line], severity: match[matcher.pattern.severity], message: match[matcher.pattern.message] };
  }

  it('识别 cc65、ca65 的“文件(行): Error: 信息”与“文件:行: Error: 信息”', () => {
    expect(parse("D:\\proj\\src\\bad.c(1): Error: Undefined symbol: 'x'")).toEqual({ file: 'D:\\proj\\src\\bad.c', line: '1', severity: 'Error', message: "Undefined symbol: 'x'" });
    expect(parse('D:\\proj\\src\\bad.s:4: Error: Illegal mnemonic')).toEqual({ file: 'D:\\proj\\src\\bad.s', line: '4', severity: 'Error', message: 'Illegal mnemonic' });
    expect(parse('src/a.c(12): Warning: Unused variable')).toMatchObject({ file: 'src/a.c', line: '12', severity: 'Warning' });
  });

  it('不是诊断的行不匹配', () => {
    expect(parse('编译 hello.c')).toBeUndefined();
    expect(parse('错误：链接失败：hello')).toBeUndefined();
  });
});
