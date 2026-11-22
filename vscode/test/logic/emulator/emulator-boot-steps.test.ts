// ------------------------------------------------------------------------
// 名称：emulator-boot-steps.test.ts
// 说明：测试模拟器 load 按目标描述执行启动步骤：nes 的卡带与 Family BASIC 键盘、sb2000 的程序、缺参
//       数与参数组合不对时的错误。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用假的调试会话，只检查发出的请求与顺序；目标描述读扩展自带的 nes 与 sb2000。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { createEmulatorHarness, createFakeClient, type EmulatorHarness } from '../../support/emulator-fixtures';

describe('模拟器 load 按目标描述', () => {
  let temp: TempDir;
  let harness: EmulatorHarness;

  beforeEach(() => {
    temp = createTempDir();
    harness = createEmulatorHarness();
  });
  afterEach(() => temp.cleanup());

  it('nes 的 cartridge：载入卡带并通电；family-basic 先接入 Family BASIC 键盘', async () => {
    const file = writeTestFile(temp.dir, 'game.nes', 'x');
    const client = createFakeClient({ 'cartridges.load': {}, 'input.setFamilyKeyboard': {} });
    await harness.connect(client);
    const loaded = await harness.run('load', { target: 'nes', file });
    assert.deepEqual([loaded.target, loaded.input, loaded.power], ['nes', 'cartridge', 'on']);
    assert.deepEqual(client.calls.slice(0, 1), [{ method: 'cartridges.load', params: { path: file, powerOn: true, resume: false } }]);
    client.calls.length = 0;
    await harness.run('load', { target: 'nes', input: 'family-basic', file });
    assert.deepEqual(client.calls.slice(0, 2).map((call) => [call.method, (call.params as { connected?: boolean; path?: string }).connected ?? (call.params as { path?: string }).path]), [['input.setFamilyKeyboard', true], ['cartridges.load', file]]);
  });

  it('sb2000 的 program：添加固件、开机、放入磁盘、跑 900 帧、回车两次、输入程序名、回车', async () => {
    const firmware = writeTestFile(temp.dir, 'bios.bin', 'x');
    const diskImage = writeTestFile(temp.dir, 'disk.img', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {} });
    await harness.connect(client);
    await harness.run('load', { target: 'sb2000', firmware, diskImage, program: 'Hello' });
    assert.deepEqual(client.calls.slice(0, 4).map((call) => call.method), ['machines.setRom', 'machines.boot', 'media.floppyInsert', 'runtime.run']);
    assert.deepEqual(client.calls[3]?.params, { frames: 900, wait: true });
    const downs = client.calls.filter((call) => call.method === 'input.key' && (call.params as { down: boolean }).down).map((call) => (call.params as { code: string }).code);
    assert.deepEqual(downs, ['Enter', 'Enter', 'KeyH', 'KeyE', 'KeyL', 'KeyL', 'KeyO', 'Enter']);
    assert.ok(client.calls.filter((call) => call.method === 'runtime.run').slice(1).every((call) => (call.params as { frames: number }).frames === 8));
  });

  it('缺少步骤要用的参数、给了 input 没给 target、同时给 target 与 machine 时报错', async () => {
    await harness.connect(createFakeClient({ 'machines.boot': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {} }));
    await assert.rejects(harness.run('load', { target: 'nes' }), (error: FcdevError) => error.code === 'EMULATOR_PARAM_INVALID' && error.message.includes('file'));
    await assert.rejects(harness.run('load', { target: 'sb2000' }), (error: FcdevError) => error.code === 'EMULATOR_PARAM_INVALID' && error.message.includes('program'));
    await assert.rejects(harness.run('load', { target: 'nes', machine: 'sb2000' }), (error: FcdevError) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(harness.run('load', { input: 'bios', file: 'a.nes' }), (error: FcdevError) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(harness.run('load', { target: 'sb2000', input: 'tape' }), (error: FcdevError) => error.code === 'PROFILE_INPUT_UNKNOWN');
  });
});
