// ------------------------------------------------------------------------
// 名称：disasm-targets-e2e.test.mjs
// 说明：端到端：用 fcdev 构建 NES 程序与 SB-DOS 程序（含多页），按内置目标描述新建反汇编工程、追踪、生成并回读；再在真实 Ember 里按同一份目标描述载入。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：构建与回读需要 cc65，缺失时整组跳过；真实 Ember 的用例还需要环境变量 FCDEV_EMULATOR、已构建的调试 SDK 与学习机 BIOS（见 sb2000-runner.mjs），缺一就跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync, rmSync } from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { runOperation } from '../src/disasm/index.mjs';
import { debugUserDataDirectory, runEmulatorOperation } from '../src/emulator/index.mjs';
import { getDiskPackager } from '../src/packagers/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';
import { findSb2000Setup } from './sb2000-runner.mjs';

/** 三个库页各放一块特征数据的 SB2000 程序 */
const BANK_MARKERS_C = [
  '#include <sb2000.h>',
  'SB_BANK_BEGIN(1)\nconst unsigned char marker1[4] = { 0xA1, 0xA1, 0xA1, 0xA1 };\nSB_BANK_END()',
  'SB_BANK_BEGIN(2)\nconst unsigned char marker2[4] = { 0xB2, 0xB2, 0xB2, 0xB2 };\nSB_BANK_END()',
  'SB_BANK_BEGIN(3)\nconst unsigned char marker3[4] = { 0xC3, 0xC3, 0xC3, 0xC3 };\nSB_BANK_END()',
  'extern const unsigned char marker1[4], marker2[4], marker3[4];',
  'int main(void) { return marker1[0] + marker2[0] + marker3[0]; }',
  '',
].join('\n');

/** 取一个当前空闲的端口 */
async function findFreePort() {
  const server = net.createServer();
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const { port } = server.address();
  await new Promise((resolve) => server.close(resolve));
  return port;
}

describe('按目标描述反汇编 fcdev 构建的程序（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;
  let projectDir;

  beforeEach(() => {
    temp = createTempDir();
    projectDir = path.join(temp.dir, 'disasm');
  });
  afterEach(() => temp.cleanup());

  /** 构建一个 fcdev 项目，返回产物路径 */
  async function build(config, fileName, source) {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify(config));
    writeTestFile(temp.dir, fileName, source);
    return (await buildPath({ pathArg: fileName.includes('/') ? fileName.split('/')[0] : fileName, cwd: temp.dir })).result.outputFile;
  }

  /** 新建工程、追踪、生成并回读，返回回读结果 */
  async function disassemble(image, params) {
    await runOperation('open', { projectDir, image, ...params });
    await runOperation('trace', { projectDir });
    await runOperation('generate', { projectDir });
    return runOperation('verify', { projectDir });
  }

  it('NES：target 为 nes，入口取自中断向量，用 fcnes.inc 的名字，回读一致', async () => {
    const image = await build({ target: 'nes' }, 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "HI"); nes_screen_on(); for (;;) { } }\n');
    const verified = await disassemble(image, { target: 'nes' });
    assert.equal(verified.ok, true, JSON.stringify(verified));
    assert.match(readFileSync(path.join(projectDir, 'src', 'main.s'), 'utf8'), /PPUSTATUS/);
    assert.deepEqual((await runOperation('status', { projectDir })).profile, { name: 'nes', input: 'cartridge', graphicsPorts: [0x2007] });
  });

  it('SB2000：target 为 sb2000，入口取自文件头的 JMP，用 sb2000.inc 的名字，回读一致', async () => {
    const image = await build({ target: 'sb2000', name: 'hello' }, 'src/hello.c', '#include <sb2000.h>\nint main(void) { sb_puts("Hi"); return 0; }\n');
    const verified = await disassemble(image, { target: 'sb2000' });
    assert.equal(verified.ok, true, JSON.stringify(verified));
    assert.match(readFileSync(path.join(projectDir, 'src', 'main.s'), 'utf8'), /HW_|SYS_|FN_|sb_/);
  });

  it('SB2000 多页程序：常驻、库页 1、栈页、库页 2 起各是一个段，回读一致', async () => {
    const image = await build({ target: 'sb2000', name: 'big', targetOptions: { banks: 3 } }, 'src/big.c', BANK_MARKERS_C);
    await runOperation('open', { projectDir, image, target: 'sb2000' });
    const layout = await runOperation('layout', { projectDir });
    assert.deepEqual(layout.segments.map((segment) => [segment.address, segment.length, segment.bank]), [
      [0x8000, 0x2000, null], [0xa000, 0x1000, 1], [0xb000, 0x1000, null], [0xa000, 0x1000, 2], [0xa000, 0x1000, 3],
    ]);
    await runOperation('trace', { projectDir });
    await runOperation('generate', { projectDir });
    assert.equal((await runOperation('verify', { projectDir })).ok, true);
  });
});

/** 真实的 Ember 与学习机 BIOS；缺任何一样时为 null */
const setup = findSb2000Setup();

describe('在真实 Ember 里按目标描述载入', { skip: setup ? false : '需要 Ember（FCDEV_EMULATOR）、cc65、已构建的调试 SDK 与 SB2000 的 BIOS' }, () => {
  let temp;
  let port;

  beforeEach(async () => {
    temp = createTempDir();
    port = await findFreePort();
  });
  afterEach(async () => {
    await runEmulatorOperation('status', { close: true, terminate: true });
    try {
      rmSync(debugUserDataDirectory(port), { recursive: true, force: true, maxRetries: 10, retryDelay: 300 });
    } catch {
      // Ember 退出需要一点时间，目录还被占用时放弃清理也不影响测试结果
    }
    temp.cleanup();
  });

  it('nes 的 cartridge：载入后停在开机状态，运行 30 帧后能截图', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "TEST"); nes_wait_vblank(); nes_screen_on(); for (;;) { } }\n');
    const { result } = await buildPath({ pathArg: 'hello.c', cwd: temp.dir });
    await runEmulatorOperation('launch', { port });
    const loaded = await runEmulatorOperation('load', { target: 'nes', file: result.outputFile });
    assert.deepEqual([loaded.target, loaded.power, loaded.mode], ['nes', 'on', 'paused']);
    assert.equal((await runEmulatorOperation('run', { frames: 30 })).stopped, 'completed');
    assert.ok((await runEmulatorOperation('screenshot', {})).imageBase64.startsWith('iVBOR'));
  });

  it('sb2000 的 program：固件开机、放入磁盘、跳过日期与时间、输入程序名，程序已装入 $8000', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello' }));
    writeTestFile(temp.dir, 'src/hello.c', '#include <sb2000.h>\nint main(void) { for (;;) { } }\n');
    const { result } = await buildPath({ pathArg: 'src', cwd: temp.dir });
    const diskImage = path.join(temp.dir, 'test.img');
    getDiskPackager('fat-floppy')({ files: [{ name: path.basename(result.outputFile), data: readFileSync(result.outputFile) }], outputFile: diskImage, options: { label: 'FCDEV' } });
    await runEmulatorOperation('launch', { port });
    const loaded = await runEmulatorOperation('load', { target: 'sb2000', firmware: setup.firmware, diskImage, program: 'HELLO' });
    assert.deepEqual([loaded.target, loaded.input, loaded.power], ['sb2000', 'program', 'on']);
    await runEmulatorOperation('run', { frames: 120 });
    const header = await runEmulatorOperation('inspect', { what: 'memory', address: '$8000', length: 5 });
    assert.match(header.text, /^8000 {2}4C [0-9A-F]{2} [0-9A-F]{2} AA 55/);
  });
});
