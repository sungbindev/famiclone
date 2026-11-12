// ------------------------------------------------------------------------
// 名称：sb2000.test.mjs
// 说明：测试 SB2000 目标：构建 C 与汇编程序，文件头、大小与内存布局，disk 命令，缺少固件的提示，以及在模拟器里运行（需要固件）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：模拟器里的用例需要环境变量 FCDEV_TEST_FIRMWARE 指向 SB2000 的 BIOS 文件（仓库里有 machines/sb2000/reference/rom/SB2K_BIOS.NES），没有时跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { runCli } from '../src/cli/index.mjs';
import { DebugClient, launchEmulator, locateEmulator } from '../src/emulator/index.mjs';
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

  it('run 缺少固件时提示怎么配置，并且不启动模拟器', async () => {
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    const stderr = [];
    const code = await runCli(['run', 'src'], { cwd: temp.dir, env: { ...process.env, FCDEV_EMULATOR: process.execPath }, stdout: { write: () => {} }, stderr: { write: (text) => stderr.push(text) } });
    assert.equal(code, 1);
    assert.match(stderr.join(''), /需要固件/);
    assert.match(stderr.join(''), /firmware/);
  });
});

/** 模拟器与固件都可用时才运行真实环境的用例 */
function findRealSetup() {
  const firmware = process.env.FCDEV_TEST_FIRMWARE;
  if (!firmware || !existsSync(firmware) || process.env.FCDEV_SKIP_GUI_TESTS === '1' || SKIP_WITHOUT_TOOLCHAIN) return null;
  try {
    return { firmware, emulator: locateEmulator() };
  } catch {
    return null;
  }
}

const realSetup = findRealSetup();

/** DirectInput 扫描码 */
const SCAN = { enter: 0x1c, h: 0x23, e: 0x12, l: 0x26, o: 0x18 };

/** 按下并松开一个键，期间运行若干帧让 BIOS 读到 */
async function tapKey(client, scancode) {
  await client.send(`key ${scancode.toString(16)} 1`);
  await client.send('run 5');
  await client.send(`key ${scancode.toString(16)} 0`);
  await client.send('run 4');
}

describe('SB2000 程序在模拟器里运行', { skip: realSetup ? false : '需要模拟器、cc65 与环境变量 FCDEV_TEST_FIRMWARE（SB2000 的 BIOS）' }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('在 DOS 提示符下输入程序名后，会执行到 main，按键后返回提示符', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello', firmware: realSetup.firmware }));
    writeTestFile(temp.dir, 'src/hello.c', HELLO_C);
    const { plan, result } = await buildPath({ pathArg: 'src', cwd: temp.dir });
    const mapText = readFileSync(path.join(plan.objectDir, 'hello.map'), 'utf8');
    const mainAddress = /_main\s+([0-9A-F]{6})/.exec(mapText)[1].slice(2);

    // 找一个空闲端口
    const net = await import('node:net');
    const port = await new Promise((resolve) => {
      const server = net.createServer().listen(0, '127.0.0.1', () => {
        const { port: free } = server.address();
        server.close(() => resolve(free));
      });
    });

    const { buildDiskImage } = await import('../src/build/index.mjs');
    const { imageFile } = buildDiskImage(plan, result);
    const { client } = await launchEmulator({ executable: realSetup.emulator.executable, romFile: realSetup.firmware, debug: true, port });
    try {
      assert.ok((await client.send(`disk ${imageFile}`)).ok);
      assert.ok((await client.send('reset')).ok);
      await client.send('run 1300', { timeoutMs: 120_000 });
      await tapKey(client, SCAN.enter);
      await client.send('run 60');
      await tapKey(client, SCAN.enter);
      await client.send('run 400');
      assert.ok((await client.send(`bp x ${mainAddress}`)).ok);
      for (const key of [SCAN.h, SCAN.e, SCAN.l, SCAN.l, SCAN.o, SCAN.enter]) await tapKey(client, key);
      const reached = await client.send('run 600', { timeoutMs: 120_000 });
      assert.match(reached.head, new RegExp(`stop=exec.*stoppc=${mainAddress}`), `应停在 main（$${mainAddress}）：${reached.head}`);
      assert.match(reached.head, /s=[CDEF][0-9A-F]/, `main 入口的栈指针应在 $C0 以上：${reached.head}`);
    } finally {
      await client.send('quit').catch(() => {});
      client.close();
    }
  });
});
