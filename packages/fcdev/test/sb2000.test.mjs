// ------------------------------------------------------------------------
// 名称：sb2000.test.mjs
// 说明：测试 SB2000 目标：构建 C 与汇编程序，文件头、大小与内存布局，disk 命令。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：在模拟器里运行 SB2000 程序的用例已删除：Ember 还没有支持学习机整机，待支持后再用调试服务补写；fcdev run 对 sb2000 目标报“暂不支持”的用例在 emulator.test.mjs。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { runCli } from '../src/cli/index.mjs';
import { listFat12, readFat12File } from '../src/packagers/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 最简单的 C 程序：显示一行字，等待按键 */
const HELLO_C = '#include <sb2000.h>\nint main(void) { sb_puts("Hello"); sb_getkey(); return 0; }\n';

/** 汇编程序：导出 _main，直接返回 DOS */
const HELLO_ASM = '.export _main\n.segment "CODE"\n_main:\n        rts\n';

describe('SB2000 目标（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello' }));
  });

  afterEach(() => temp.cleanup());

  /** 构建并返回结果 */
  async function build(pathArg) {
    const { result } = await buildPath({ pathArg, cwd: temp.dir });
    return result;
  }

  it('C 程序构建出 DOS 程序：大写 8.3 名字，文件头 4C lo hi AA 55，入口跳到启动代码', async () => {
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    const result = await build('src');
    assert.equal(path.basename(result.outputFile), 'HELLO.EXE');
    const image = readFileSync(result.outputFile);
    assert.equal(image[0], 0x4c);
    assert.deepEqual([...image.subarray(3, 5)], [0xaa, 0x55]);
    const entry = image.readUInt16LE(1);
    assert.ok(entry > 0x8007 && entry < 0x8000 + image.length, `入口 $${entry.toString(16)} 应在文件头之后`);
    assert.ok(image.length < 1024, `最小程序应小于 1K，实际 ${image.length}`);
  });

  it('汇编程序构建成功', async () => {
    writeTestFile(temp.dir, 'src/hello.s', HELLO_ASM);
    const result = await build('src/hello.s');
    assert.equal(readFileSync(result.outputFile)[0], 0x4c);
  });

  it('运行库一个函数一个目标文件，链接时只带入程序用到的函数', async () => {
    writeTestFile(temp.dir, 'src/hello.c', `#include <sb2000.h>\nint main(void)\n{\n    sb_clock_get_time(0);\n    return 0;\n}\n`);
    await build('src');
    const map = readFileSync(path.join(temp.dir, 'build', 'obj', 'sb2000', 'hello', 'hello.map'), 'utf8');
    assert.match(map, /sb-clock-get-time\.o/);
    for (const unused of ['sb-disk-format-track', 'sb-file-open', 'sb-image-show', 'sb-printer-text', 'sb-sound-play-from']) {
      assert.doesNotMatch(map, new RegExp(`${unused}\\.o`), `${unused} 没被用到，不应链接进程序`);
    }
  });

  it('C 栈在窗口顶端，BSS 紧跟数据区且不占文件空间', async () => {
    writeTestFile(temp.dir, 'src/big.c', 'static unsigned char buffer[8000];\nint main(void) { buffer[0] = 1; return buffer[0]; }\n');
    const result = await build('src/big.c');
    assert.ok(result.size < 1024, `8000 字节的 BSS 不应写进文件，实际 ${result.size}`);
    const map = readFileSync(path.join(temp.dir, 'build', 'obj', 'sb2000', 'big', 'big.map'), 'utf8');
    assert.match(map, /__STACKSTART__\s+00C000/);
    assert.match(map, /__ZPSTART__\s+000060/);
  });

  it('程序超过 16K 窗口时链接失败', async () => {
    writeTestFile(temp.dir, 'src/huge.c', 'const unsigned char table[16000] = { 1 };\nint main(void) { return table[0]; }\n');
    await assert.rejects(build('src/huge.c'), (error) => error.code === 'TOOL_FAILED');
  });

  /** 三个库页各放一块特征数据的程序 */
  const BANK_MARKERS_C = [
    '#include <sb2000.h>',
    'SB_BANK_BEGIN(1)\nconst unsigned char marker1[4] = { 0xA1, 0xA1, 0xA1, 0xA1 };\nSB_BANK_END()',
    'SB_BANK_BEGIN(2)\nconst unsigned char marker2[4] = { 0xB2, 0xB2, 0xB2, 0xB2 };\nSB_BANK_END()',
    'SB_BANK_BEGIN(3)\nconst unsigned char marker3[4] = { 0xC3, 0xC3, 0xC3, 0xC3 };\nSB_BANK_END()',
    'extern const unsigned char marker1[4], marker2[4], marker3[4];',
    'int main(void) { return marker1[0] + marker2[0] + marker3[0]; }',
    '',
  ].join('\n');

  it('多页程序：库页个数决定文件页数，常驻 2 页、库页 1、栈页、库页 2 起依次排列', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'big', targetOptions: { banks: 3 } }));
    writeTestFile(temp.dir, 'src/big.c', BANK_MARKERS_C);
    const image = readFileSync((await build('src')).outputFile);
    assert.equal(image.length, 4096 * (3 + 3), '2 页常驻 + 库页 1 + 栈页 + 库页 2、3');
    assert.deepEqual([...image.subarray(3, 5)], [0xaa, 0x55]);
    const pageOf = (marker) => Math.floor(image.indexOf(Buffer.alloc(4, marker)) / 4096);
    assert.equal(pageOf(0xa1), 2, '库页 1 在文件第 2 页');
    assert.equal(pageOf(0xb2), 4, '库页 2 在文件第 4 页');
    assert.equal(pageOf(0xc3), 5, '库页 3 在文件第 5 页');
    assert.ok(image.subarray(3 * 4096, 4 * 4096).every((byte) => byte === 0), '第 3 页是栈页，不放程序内容');
  });

  it('库页个数不够时链接失败并提示加大 banks', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'big', targetOptions: { banks: 2 } }));
    writeTestFile(temp.dir, 'src/big.c', BANK_MARKERS_C);
    await assert.rejects(build('src'), (error) => error.code === 'TOOL_FAILED' && error.hint.includes('banks'));
  });

  it('没有设置库页个数时，程序里用库页会链接失败', async () => {
    writeTestFile(temp.dir, 'src/big.c', BANK_MARKERS_C);
    await assert.rejects(build('src'), (error) => error.code === 'TOOL_FAILED' && error.hint.includes('banks'));
  });

  it('库页个数超过上限 113 时报错', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'big', targetOptions: { banks: 114 } }));
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    await assert.rejects(build('src'), (error) => error.code === 'TARGET_OPTION_INVALID');
  });

  it('程序名超过 8 个字符时报错', async () => {
    writeTestFile(temp.dir, 'src/toolongname.c', HELLO_C);
    await assert.rejects(build('src/toolongname.c'), (error) => error.code === 'NAME_INVALID');
  });

  it('disk 命令生成软盘镜像，里面的 HELLO.EXE 与产物一致', async () => {
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    const stdout = [];
    const code = await runCli(['disk', 'src'], { cwd: temp.dir, env: process.env, stdout: { write: (text) => stdout.push(text) }, stderr: { write: () => {} } });
    assert.equal(code, 0);
    const image = readFileSync(path.join(temp.dir, 'build', 'sb2000', 'HELLO.img'));
    const [entry] = listFat12(image);
    assert.equal(entry.name, 'HELLO.EXE');
    assert.deepEqual(readFat12File(image, entry), readFileSync(path.join(temp.dir, 'build', 'sb2000', 'HELLO.EXE')));
  });

  it('不支持磁盘镜像的目标运行 disk 时报错', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'a.c', 'int main(void) { for (;;) { } }\n');
    const stderr = [];
    const code = await runCli(['disk', 'a.c'], { cwd: temp.dir, env: process.env, stdout: { write: () => {} }, stderr: { write: (text) => stderr.push(text) } });
    assert.equal(code, 1);
    assert.match(stderr.join(''), /不支持磁盘镜像/);
  });
});
