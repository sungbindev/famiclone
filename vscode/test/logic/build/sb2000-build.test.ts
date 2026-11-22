// ------------------------------------------------------------------------
// 名称：sb2000-build.test.ts
// 说明：测试 SB2000 目标：构建 C 与汇编程序，文件头、大小与内存布局，多页程序，磁盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用真实 cc65，本机没有组装工具链时整体跳过；在模拟器里运行 SB2000 程序的用例
//       随模拟器功能迁入。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { listFat12, readFat12File } from '../../../src/shared/packaging/fat12-image';
import { HAS_TOOLCHAIN, createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

/** 真实构建较慢，单个用例的超时时间。 */
const TIMEOUT_MS = 120_000;

/** 最简单的 C 程序：显示一行字，等待按键。 */
const HELLO_C = '#include <sb2000.h>\nint main(void) { sb_puts("Hello"); sb_getkey(); return 0; }\n';

/** 汇编程序：导出 _main，直接返回 DOS。 */
const HELLO_ASM = '.export _main\n.segment "CODE"\n_main:\n        rts\n';

/** 三个库页各放一块特征数据的程序。 */
const BANK_MARKERS_C = [
  '#include <sb2000.h>',
  'SB_BANK_BEGIN(1)\nconst unsigned char marker1[4] = { 0xA1, 0xA1, 0xA1, 0xA1 };\nSB_BANK_END()',
  'SB_BANK_BEGIN(2)\nconst unsigned char marker2[4] = { 0xB2, 0xB2, 0xB2, 0xB2 };\nSB_BANK_END()',
  'SB_BANK_BEGIN(3)\nconst unsigned char marker3[4] = { 0xC3, 0xC3, 0xC3, 0xC3 };\nSB_BANK_END()',
  'extern const unsigned char marker1[4], marker2[4], marker3[4];',
  'int main(void) { return marker1[0] + marker2[0] + marker3[0]; }',
  ''
].join('\n');

describe.skipIf(!HAS_TOOLCHAIN)('SB2000 目标（需要 cc65）', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello' }));
  });

  afterEach(() => temp.cleanup());

  /** 构建并返回结果。 */
  async function build(pathArg: string): Promise<Awaited<ReturnType<typeof service.build>>> {
    return service.build({ pathArg, cwd: temp.dir });
  }

  /** 中间文件目录里某个程序的映射文件。 */
  function readMap(name: string): string {
    return readFileSync(path.join(temp.dir, 'build', 'obj', 'sb2000', name, `${name}.map`), 'utf8');
  }

  it('C 程序构建出 DOS 程序：大写 8.3 名字，文件头 4C lo hi AA 55，入口跳到启动代码', async () => {
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    const { result } = await build('src');
    expect(path.basename(result.outputFile)).toBe('HELLO.EXE');
    const image = readFileSync(result.outputFile);
    expect(image[0]).toBe(0x4c);
    expect([...image.subarray(3, 5)]).toEqual([0xaa, 0x55]);
    const entry = image.readUInt16LE(1);
    expect(entry).toBeGreaterThan(0x8007);
    expect(entry).toBeLessThan(0x8000 + image.length);
    expect(image.length).toBeLessThan(1024);
  });

  it('汇编程序构建成功', async () => {
    writeTestFile(temp.dir, 'src/hello.s', HELLO_ASM);
    const { result } = await build('src/hello.s');
    expect(readFileSync(result.outputFile)[0]).toBe(0x4c);
  });

  it('运行库一个函数一个目标文件，链接时只带入程序用到的函数', async () => {
    writeTestFile(temp.dir, 'src/hello.c', '#include <sb2000.h>\nint main(void)\n{\n    sb_clock_get_time(0);\n    return 0;\n}\n');
    await build('src');
    const map = readMap('hello');
    expect(map).toMatch(/sb-clock-get-time\.o/);
    for (const unused of ['sb-disk-format-track', 'sb-file-open', 'sb-image-show', 'sb-printer-text', 'sb-sound-play-from']) {
      expect(map, `${unused} 没被用到，不应链接进程序`).not.toMatch(new RegExp(`${unused}\\.o`));
    }
  });

  it('C 栈在窗口顶端，BSS 紧跟数据区且不占文件空间', async () => {
    writeTestFile(temp.dir, 'src/big.c', 'static unsigned char buffer[8000];\nint main(void) { buffer[0] = 1; return buffer[0]; }\n');
    const { result } = await build('src/big.c');
    expect(result.size).toBeLessThan(1024);
    const map = readMap('big');
    expect(map).toMatch(/__STACKSTART__\s+00C000/);
    expect(map).toMatch(/__ZPSTART__\s+000060/);
  });

  it('程序超过 16K 窗口时链接失败', async () => {
    writeTestFile(temp.dir, 'src/huge.c', 'const unsigned char table[16000] = { 1 };\nint main(void) { return table[0]; }\n');
    await expect(build('src/huge.c')).rejects.toMatchObject({ code: 'TOOL_FAILED' });
  });

  it('多页程序：库页个数决定文件页数，常驻 2 页、库页 1、栈页、库页 2 起依次排列', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'big', targetOptions: { banks: 3 } }));
    writeTestFile(temp.dir, 'src/big.c', BANK_MARKERS_C);
    const image = readFileSync((await build('src')).result.outputFile);
    expect(image.length).toBe(4096 * (3 + 3));
    expect([...image.subarray(3, 5)]).toEqual([0xaa, 0x55]);
    const pageOf = (marker: number): number => Math.floor(image.indexOf(Buffer.alloc(4, marker)) / 4096);
    expect(pageOf(0xa1)).toBe(2);
    expect(pageOf(0xb2)).toBe(4);
    expect(pageOf(0xc3)).toBe(5);
    expect(image.subarray(3 * 4096, 4 * 4096).every((byte) => byte === 0)).toBe(true);
  });

  it('库页个数不够，或没有设置库页个数时，链接失败并提示加大 banks', async () => {
    writeTestFile(temp.dir, 'src/big.c', BANK_MARKERS_C);
    await expect(build('src')).rejects.toMatchObject({ code: 'TOOL_FAILED', hint: expect.stringContaining('banks') });
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'big', targetOptions: { banks: 2 } }));
    await expect(build('src')).rejects.toMatchObject({ code: 'TOOL_FAILED', hint: expect.stringContaining('banks') });
  });

  it('库页个数超过上限 113 时报错', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'big', targetOptions: { banks: 114 } }));
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    await expect(build('src')).rejects.toMatchObject({ code: 'TARGET_OPTION_INVALID' });
  });

  it('程序名超过 8 个字符时报错', async () => {
    writeTestFile(temp.dir, 'src/toolongname.c', HELLO_C);
    await expect(build('src/toolongname.c')).rejects.toMatchObject({ code: 'NAME_INVALID' });
  });

  it('磁盘镜像里的 HELLO.EXE 与产物一致', async () => {
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    const outcome = await build('src');
    const { imageFile } = service.buildDisk(outcome);
    expect(imageFile).toBe(path.join(temp.dir, 'build', 'sb2000', 'HELLO.img'));
    const image = readFileSync(imageFile);
    const [entry] = listFat12(image);
    expect(entry?.name).toBe('HELLO.EXE');
    expect(readFat12File(image, entry!)).toEqual(readFileSync(outcome.result.outputFile));
  });

  it('不支持磁盘镜像的目标生成镜像时报错', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'a.c', 'int main(void) { for (;;) { } }\n');
    const outcome = await build('a.c');
    expect(() => service.buildDisk(outcome)).toThrow(/不支持磁盘镜像/);
  });
});
