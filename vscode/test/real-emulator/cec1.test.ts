// ------------------------------------------------------------------------
// 名称：cec1.test.ts
// 说明：在真实 Ember 里测试中华学习机 CEC-I 目标：模板的示例程序构建后放进 DOS 3.3 系统盘的副本，
//       开机、输入 BRUN 与程序名运行，屏幕上显示两行文字；按键后程序用热启动入口回到 DOS 的
//       命令行（提示符出现，CPU 停在等键循环）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：需要环境变量 FCDEV_TEST_EMULATOR 指向 Ember、cc65 工具链、CEC-I 的 ROM（环境变量
//       FCDEV_TEST_CEC1_FIRMWARE，没有时用仓库里的 ember/roms/cec1_v1.21.rom）与仓库里的
//       DOS 3.3 系统盘（reference/floppy-images/cec1/DOS33.DSK），缺一就跳过，设置
//       FCDEV_SKIP_GUI_TESTS=1 也会跳过；只结束自己启动的 Ember。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { copyFileSync, existsSync, mkdirSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { DebugSession } from '../../src/contracts/debug-client/debug-session';
import { launchEmulator } from '../../src/logic/emulator/emulator-launcher';
import { enterProgram, runFrames, tapKey } from '../../src/logic/emulator/machine-session';
import { createRealBuildEnvironment, createTempDir, EXTENSION_DIRECTORY, writeTestFile, type TempDir } from '../support/build-fixtures';
import { CAN_RUN_REAL_EMBER, cleanUserData, createRealEmulator, REAL_EMBER, type RealEmulator } from '../support/real-emulator';

/** 仓库里的 CEC-I ROM（1.21 版），没有设置 FCDEV_TEST_CEC1_FIRMWARE 时使用。 */
const REPOSITORY_FIRMWARE = path.resolve(EXTENSION_DIRECTORY, '..', 'ember', 'roms', 'cec1_v1.21.rom');

/** 仓库里的 Apple DOS 3.3 系统盘，测试程序放在它的副本上。 */
const SYSTEM_DISK = path.resolve(EXTENSION_DIRECTORY, '..', 'reference', 'floppy-images', 'cec1', 'DOS33.DSK');

/** CEC-I 的 ROM：环境变量 FCDEV_TEST_CEC1_FIRMWARE，没有时用仓库里的；不存在时为 undefined。 */
const FIRMWARE: string | undefined = (() => {
  const firmware = process.env.FCDEV_TEST_CEC1_FIRMWARE ?? REPOSITORY_FIRMWARE;
  return existsSync(firmware) ? firmware : undefined;
})();

/** 需要 Ember、cc65、ROM 与系统盘的测试是否可以运行。 */
const CAN_RUN = CAN_RUN_REAL_EMBER && FIRMWARE !== undefined && existsSync(SYSTEM_DISK);

/** 模板里的程序名。 */
const PROGRAM_NAME = 'demo';

/** 监控程序等键循环（KEYIN）所在的地址范围：命令行等待输入时 CPU 在这里。 */
const KEYIN_FIRST = 0xfd1b;
const KEYIN_LAST = 0xfd28;

/**
 * 读文本页第 1 页的一行（40 个字符），反相与闪烁的字符按对应的 ASCII 码显示。
 * @param client 调试会话。
 * @param row 行号，0 到 23。
 * @returns 去掉末尾空格的文字。
 */
async function readTextRow(client: DebugSession, row: number): Promise<string> {
  const address = 0x0400 + (row % 8) * 0x80 + Math.floor(row / 8) * 0x28;
  const { data } = await client.request('inspector.readMemory', { space: 'cpu', address, length: 40 });
  assert.ok(data, '没有读到文本页');
  return Array.from(Buffer.from(data), (byte) => String.fromCharCode((byte & 0x7f) < 0x20 ? (byte & 0x7f) + 0x40 : byte & 0x7f)).join('').trimEnd();
}

/**
 * 读 CPU 的程序计数器。
 * @param client 调试会话。
 * @returns 地址。
 */
async function readProgramCounter(client: DebugSession): Promise<number> {
  const { states } = await client.request('inspector.getState', { module: 'cpu' });
  const field = states[0]?.fields.find((item) => item.name === 'pc');
  assert.ok(field, '没有取到 pc');
  return field.value;
}

describe.skipIf(!CAN_RUN)('CEC-I 程序在真实 Ember 里运行（需要 FCDEV_TEST_EMULATOR、cc65、ROM 与 DOS 3.3 系统盘）', { timeout: 180_000 }, () => {
  let temp: TempDir;
  let emulator: RealEmulator;
  let pid: number | undefined;
  let client: DebugSession | undefined;

  beforeEach(async () => {
    temp = createTempDir();
    emulator = await createRealEmulator();
    pid = undefined;
    client = undefined;
  });
  afterEach(() => {
    client?.close();
    if (pid !== undefined) emulator.environment.processes.terminate(pid);
    cleanUserData(emulator.processes, emulator.port);
    temp.cleanup();
  });

  it('示例程序：显示两行文字；按键后回到 DOS 的命令行，提示符出现，CPU 停在等键循环', async () => {
    assert.ok(REAL_EMBER && FIRMWARE, '没有可用的 Ember 与 CEC-I 的 ROM');
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'cec1', name: PROGRAM_NAME, firmware: FIRMWARE, diskImage: SYSTEM_DISK, diskName: 'cec1-test.dsk' }));
    mkdirSync(path.join(temp.dir, 'src'), { recursive: true });
    copyFileSync(path.join(EXTENSION_DIRECTORY, 'templates', 'cec1-asm', 'src', 'hello.s'), path.join(temp.dir, 'src', 'hello.s'));
    const { disk } = await createRealBuildEnvironment().service.build({ pathArg: 'src', cwd: temp.dir });
    assert.ok(disk, '构建没有生成磁盘镜像');

    const launched = await launchEmulator(emulator.environment, {
      executable: REAL_EMBER,
      firmwareDisk: { machine: 'cec1', firmware: FIRMWARE, diskImage: disk.imageFile },
      debug: true,
      port: emulator.port
    });
    assert.ok(launched.client !== null && launched.pid !== undefined, 'Ember 没有给出调试连接');
    const session = launched.client;
    client = session;
    pid = launched.pid;

    await enterProgram(session, PROGRAM_NAME.toUpperCase(), 'cec1');
    await runFrames(session, 150);
    assert.equal(await readTextRow(session, 0), 'HELLO, CEC-I!');
    assert.equal(await readTextRow(session, 1), 'PRESS ANY KEY TO EXIT.');

    await tapKey(session, 'Space', 8, 8);
    await runFrames(session, 400);
    const pc = await readProgramCounter(session);
    assert.ok(pc >= KEYIN_FIRST && pc <= KEYIN_LAST, `程序没有回到命令行：pc = $${pc.toString(16)}`);
    const rows = await Promise.all(Array.from({ length: 8 }, (_, row) => readTextRow(session, row)));
    assert.ok(rows.some((row) => row.startsWith(']')), `屏幕上没有提示符：${JSON.stringify(rows)}`);
  });
});
