// ------------------------------------------------------------------------
// 名称：real-ember.test.ts
// 说明：在真实 Ember 里测试：NES 程序调试运行后停在开机状态、运行 60 帧、截图、调试连接能连上同一个
//       Ember；记录 CDL、导入反汇编工程再追踪的闭环；没有运行的 Ember 时连接失败的提示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：真实 Ember 的用例会短暂打开窗口，需要环境变量 FCDEV_TEST_EMULATOR 指向 Ember 的可执行文件、
//       cc65工具链，缺一就跳过，设置 FCDEV_SKIP_GUI_TESTS=1 也会跳过；连接失败的用例不需要 Ember。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import { CoreEmulatorOperations } from '../../src/logic/emulator/core-emulator-operations';
import type { FcdevError } from '../../src/shared/errors/fcdev-error';
import { buildPath, runOperation } from '../support/disasm-fixtures';
import { createTempDir, writeTestFile, type TempDir } from '../support/build-fixtures';
import { CAN_RUN_REAL_EMBER, cleanUserData, createRealEmulator, createRealEnvironment, findFreePort, type RealEmulator } from '../support/real-emulator';

/** 一个小的 NES 程序：显示 TEST 后死循环。 */
const NES_HELLO = '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "TEST"); nes_wait_vblank(); nes_screen_on(); for (;;) { } }\n';

describe('没有运行的 Ember', () => {
  it('连接失败，并提示用 emu_launch；没有会话', async () => {
    const { environment } = createRealEnvironment(undefined, await findFreePort());
    const operations = new CoreEmulatorOperations(environment);
    await assert.rejects(operations.run('connect', { port: 1 }), (error: FcdevError) => error.code === 'EMULATOR_CONNECT_FAILED' && error.hint.includes('emu_launch'));
    assert.deepEqual(await operations.run('status', {}), { connected: false });
  });
});

describe.skipIf(!CAN_RUN_REAL_EMBER)('真实 Ember（需要 FCDEV_TEST_EMULATOR 与 cc65）', { timeout: 180_000 }, () => {
  let temp: TempDir;
  let emulator: RealEmulator;
  let pid: number | undefined;

  beforeEach(async () => {
    temp = createTempDir();
    emulator = await createRealEmulator();
    pid = undefined;
  });
  afterEach(async () => {
    await emulator.operations.run('status', { close: true, terminate: true });
    if (pid !== undefined) emulator.environment.processes.terminate(pid);
    cleanUserData(emulator.processes, emulator.port);
    temp.cleanup();
  });

  it('NES 程序调试运行后停在开机状态，运行 60 帧后能截图，另一个调试连接能连上同一个 Ember', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'hello.c', NES_HELLO);
    const launched = await emulator.runner.run({ build: { pathArg: 'hello.c', cwd: temp.dir }, debug: true });
    pid = launched.pid;
    assert.deepEqual([launched.debug, launched.port, launched.firmwareDisk], [true, emulator.port, false]);

    const connected = await emulator.operations.run('connect', {});
    assert.deepEqual([(connected.runtime as { power: string; mode: string }).power, (connected.runtime as { mode: string }).mode], ['on', 'paused']);
    const run = await emulator.operations.run('run', { frames: 60 });
    assert.deepEqual([run.stopped, run.frames], ['completed', 60]);
    const shot = await emulator.operations.run('screenshot', {});
    assert.ok(String(shot.imageBase64).startsWith('iVBOR'));

    const other = await emulator.environment.connector.connectToEmber({ userDataDirectory: emulator.processes.userDataDirectory(emulator.port) });
    try {
      assert.equal((await other.request('runtime.status', {})).power, 'on');
    } finally {
      other.close();
    }
  });

  it('启动并载入 NES 程序，记录 CDL 后导入反汇编工程，再追踪得到代码', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'hello.c', NES_HELLO);
    const { result } = await buildPath({ pathArg: 'hello.c', cwd: temp.dir });

    const launched = await emulator.operations.run('launch', {});
    assert.equal((launched.runtime as { power: string }).power, 'off');
    const loaded = await emulator.operations.run('load', { file: result.outputFile });
    assert.equal(loaded.power, 'on');
    await emulator.operations.run('record', { action: 'start', kind: 'cdl' });
    assert.equal((await emulator.operations.run('run', { frames: 30 })).stopped, 'completed');
    const cdl = await emulator.operations.run('record', { action: 'stop', kind: 'cdl' });
    assert.ok(Number(cdl.codeBytes) > 0 && existsSync(String(cdl.file)), `CDL 没有记到代码：${JSON.stringify(cdl)}`);

    const projectDir = path.join(temp.dir, 'project');
    await runOperation('open', { projectDir, image: result.outputFile });
    const imported = await runOperation('importRecord', { projectDir, kind: 'cdl', file: cdl.file });
    assert.ok(Number(imported.entriesAdded) > 0, JSON.stringify(imported));
    assert.ok(Number((await runOperation('trace', { projectDir })).instructions) > 0);
    assert.ok(String((await emulator.operations.run('screenshot', {})).imageBase64).startsWith('iVBOR'));
  });
});
