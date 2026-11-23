// ------------------------------------------------------------------------
// 名称：build-integration.test.ts
// 说明：用真实 cc65 构建：NES 程序（含增量、错误提示），学习机程序里的汉字被转成 GB2312
//       字节，整机 ROM 工程覆盖底固件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：本机没有组装工具链时整体跳过；每个用例使用独立的临时项目。
// ------------------------------------------------------------------------

import { existsSync, readFileSync, utimesSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import {
  HAS_TOOLCHAIN,
  createRealBuildEnvironment,
  createRecordingReporter,
  createTempDir,
  writeTestFile,
  type TempDir
} from '../../support/build-fixtures';

/** 真实构建较慢，单个用例的超时时间。 */
const TIMEOUT_MS = 120_000;

/** 最小可链接的 NES C 程序。 */
const NES_MAIN = 'int main(void) { for (;;) { } }\n';

/** “你好”的 GB2312 内码与 UTF-8 编码。 */
const NI_HAO = Buffer.from([0xc4, 0xe3, 0xba, 0xc3]);
const NI_HAO_UTF8 = Buffer.from('你好', 'utf8');

/** 把文件的修改时间推到未来，模拟编辑。 */
function touch(file: string): void {
  const future = new Date(Date.now() + 5000);
  utimesSync(file, future, future);
}

describe.skipIf(!HAS_TOOLCHAIN)('用真实 cc65 构建 NES 程序', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
  });

  afterEach(() => temp.cleanup());

  /** 构建并返回结果与工具输出。 */
  async function build(pathArg: string): Promise<{ result: Awaited<ReturnType<typeof service.build>>['result']; output: string }> {
    const { reporter, output } = createRecordingReporter();
    const { result } = await service.build({ pathArg, cwd: temp.dir }, reporter);
    return { result, output: output() };
  }

  it('C 程序构建出 iNES 文件', async () => {
    writeTestFile(temp.dir, 'src/hello.c', NES_MAIN);
    const { result } = await build('src/hello.c');
    expect(readFileSync(result.outputFile).subarray(0, 4).toString('latin1')).toBe('NES\x1a');
    expect(result.compiled).toBe(1);
  });

  it('汇编程序构建成功（导出 _main）', async () => {
    writeTestFile(temp.dir, 'src/hello.s', '.export _main\n.segment "CODE"\n_main:\n@loop: jmp @loop\n');
    expect(existsSync((await build('src/hello.s')).result.outputFile)).toBe(true);
  });

  it('目录构建把 C 与汇编文件链接成一个程序', async () => {
    writeTestFile(temp.dir, 'game/main.c', 'extern void spin(void);\nint main(void) { spin(); return 0; }\n');
    writeTestFile(temp.dir, 'game/spin.s', '.export _spin\n.segment "CODE"\n_spin:\n@loop: jmp @loop\n');
    const { result } = await build('game');
    expect(path.basename(result.outputFile)).toBe('game.nes');
    expect(result.compiled).toBe(2);
  });

  it('第二次构建沿用未改动的文件，改动源文件后只重编它', async () => {
    const main = writeTestFile(temp.dir, 'game/main.c', 'extern void spin(void);\nint main(void) { spin(); return 0; }\n');
    writeTestFile(temp.dir, 'game/spin.s', '.export _spin\n.segment "CODE"\n_spin:\n@loop: jmp @loop\n');
    await build('game');
    const second = await build('game');
    expect(second.result).toMatchObject({ compiled: 0, reused: 2 });
    touch(main);
    expect((await build('game')).result).toMatchObject({ compiled: 1, reused: 1 });
  });

  it('改动被包含的头文件后重编依赖它的 C 文件', async () => {
    const header = writeTestFile(temp.dir, 'game/value.h', '#define VALUE 1\n');
    writeTestFile(temp.dir, 'game/main.c', '#include "value.h"\nint main(void) { return VALUE; }\n');
    await build('game');
    touch(header);
    expect((await build('game')).result.compiled).toBe(1);
  });

  it('改动目标选项后全部重编', async () => {
    writeTestFile(temp.dir, 'src/hello.c', NES_MAIN);
    await build('src/hello.c');
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', targetOptions: { font: false } }));
    expect((await build('src/hello.c')).result.compiled).toBe(1);
  });

  it('C 语法错误时失败，工具输出原样透传', async () => {
    writeTestFile(temp.dir, 'src/bad.c', 'int main(void) { return x; }\n');
    const { reporter, output } = createRecordingReporter();
    await expect(service.build({ pathArg: 'src/bad.c', cwd: temp.dir }, reporter)).rejects.toMatchObject({ code: 'TOOL_FAILED' });
    expect(output()).toMatch(/bad\.c\(1\): Error: Undefined symbol: 'x'|bad\.c:1: Error/);
  });

  it('缺少 main 时给出提示', async () => {
    writeTestFile(temp.dir, 'src/nomain.c', 'int value;\n');
    await expect(service.build({ pathArg: 'src/nomain.c', cwd: temp.dir })).rejects.toMatchObject({
      code: 'TOOL_FAILED',
      hint: expect.stringContaining('没有找到 main')
    });
  });

  it('目录里有两个 main 时给出提示', async () => {
    writeTestFile(temp.dir, 'game/a.c', 'int main(void) { return 0; }\n');
    writeTestFile(temp.dir, 'game/b.c', 'int main(void) { return 1; }\n');
    await expect(service.build({ pathArg: 'game', cwd: temp.dir })).rejects.toMatchObject({
      code: 'TOOL_FAILED',
      hint: expect.stringContaining('多个 main')
    });
  });
});

describe.skipIf(!HAS_TOOLCHAIN)('用真实 cc65 构建含汉字的源文件', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  /** 写入项目配置。 */
  function writeConfig(target: string, name: string): void {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target, name }));
  }

  it('学习机的 C 程序里直接写汉字，程序里是 GB2312 字节', async () => {
    writeConfig('sb2000', 'hello');
    writeTestFile(temp.dir, 'src/hello.c', '#include <sb2000.h>\n/* 你好 */\nint main(void) { sb_puts("你好"); return 0; }\n');
    const image = readFileSync((await service.build({ pathArg: 'src', cwd: temp.dir })).result.outputFile);
    expect(image.includes(NI_HAO)).toBe(true);
    expect(image.includes(NI_HAO_UTF8)).toBe(false);
  });

  it('学习机的汇编程序里直接写汉字，程序里是 GB2312 字节', async () => {
    writeConfig('sb2000', 'hello');
    writeTestFile(temp.dir, 'src/hello.s', '.export _main\n.segment "CODE"\n_main:\n        rts\n.segment "RODATA"\n        .byte "你好", 0\n');
    const image = readFileSync((await service.build({ pathArg: 'src/hello.s', cwd: temp.dir })).result.outputFile);
    expect(image.includes(NI_HAO)).toBe(true);
    expect(image.includes(NI_HAO_UTF8)).toBe(false);
  });

  it('没有声明编码的目标不转码', async () => {
    writeConfig('nes', 'game');
    writeTestFile(temp.dir, 'src/game.c', 'const char text[] = "你好";\nint main(void) { return text[0]; }\n');
    const image = readFileSync((await service.build({ pathArg: 'src/game.c', cwd: temp.dir })).result.outputFile);
    expect(image.includes(NI_HAO_UTF8)).toBe(true);
  });

  it('C 编译错误指向原文件和原行号，不出现转码副本的名字', async () => {
    writeConfig('sb2000', 'bad');
    writeTestFile(temp.dir, 'src/bad.c', '/* 你好 */\nint main(void) { return x; }\n');
    const { reporter, output } = createRecordingReporter();
    await expect(service.build({ pathArg: 'src/bad.c', cwd: temp.dir }, reporter)).rejects.toMatchObject({ code: 'TOOL_FAILED' });
    expect(output()).toMatch(/bad\.c:2: Error/);
    expect(output()).not.toMatch(/gb2312/);
  });

  it('汇编错误指向原文件和原行号，不出现转码副本的名字', async () => {
    writeConfig('sb2000', 'bad');
    writeTestFile(temp.dir, 'src/bad.s', '; 你好\n.export _main\n_main:\n        bogus\n');
    const { reporter, output } = createRecordingReporter();
    await expect(service.build({ pathArg: 'src/bad.s', cwd: temp.dir }, reporter)).rejects.toMatchObject({ code: 'TOOL_FAILED' });
    expect(output()).toMatch(/bad\.s:4: Error/);
    expect(output()).not.toMatch(/gb2312/);
  });

  it('字符不在 GB2312 里时构建失败并指出位置', async () => {
    writeConfig('sb2000', 'bad');
    writeTestFile(temp.dir, 'src/bad.c', 'int main(void) { return 0; }\n/* 😀 */\n');
    await expect(service.build({ pathArg: 'src/bad.c', cwd: temp.dir })).rejects.toMatchObject({
      code: 'SOURCE_CHAR_UNSUPPORTED',
      message: expect.stringContaining(':2:4：')
    });
  });
});

describe.skipIf(!HAS_TOOLCHAIN)('用真实 cc65 构建整机 ROM 工程', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  /** 底固件：64K，每个字节是 $EE。 */
  const baseFirmware = new Uint8Array(0x10000).fill(0xee);

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'base.bin', baseFirmware);
    writeTestFile(temp.dir, 'src/rom.s', '.segment "CODE"\n        lda #$42\n.segment "VECTORS"\n        .word $8000, $8000, $8000\n');
  });

  afterEach(() => temp.cleanup());

  it('用目标自带的链接配置链接，输出覆盖到底固件的 offset 处，其余字节不变', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'rom', name: 'demo', firmware: 'base.bin', targetOptions: { machine: 'demo', offset: 0x8000, size: 0x10000 } }));
    const { result } = await service.build({ pathArg: 'src', cwd: temp.dir });
    const image = readFileSync(result.outputFile);
    expect(path.basename(result.outputFile)).toBe('demo.bin');
    expect(image.length).toBe(0x10000);
    expect(image.subarray(0, 0x8000).every((byte) => byte === 0xee)).toBe(true);
    expect([...image.subarray(0x8000, 0x8002)]).toEqual([0xa9, 0x42]);
    expect([...image.subarray(0x8000 + 0x7ffa, 0x8000 + 0x8000)]).toEqual([0x00, 0x80, 0x00, 0x80, 0x00, 0x80]);
  });

  it('没有 firmware 时输出就是整块固件，offset 不为 0 时报缺少底固件', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'rom', targetOptions: { size: 0x8000 } }));
    expect(readFileSync((await service.build({ pathArg: 'src', cwd: temp.dir })).result.outputFile).length).toBe(0x8000);
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'rom', targetOptions: { offset: 0x8000 } }));
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toMatchObject({ code: 'PACKAGE_BASE_MISSING' });
  });

  it('firmware 指向的文件不存在时报错', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'rom', firmware: 'missing.bin', targetOptions: { offset: 0x8000 } }));
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toMatchObject({ code: 'FIRMWARE_MISSING' });
  });
});
