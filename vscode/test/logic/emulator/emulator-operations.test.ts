// ------------------------------------------------------------------------
// 名称：emulator-operations.test.ts
// 说明：测试 Copilot 模拟器工具对应的操作：会话、连接、运行、断点、单步、检查、截图、输入、记录、载
//       入。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用假的调试会话（只记录请求、按方法返回预设的结果），不启动 Ember；真实 Ember 的用例在
//       test/real-emulator。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import { EMULATOR_API_VERSION, EMULATOR_OPERATION_NAMES } from '../../../src/logic/emulator/core-emulator-operations';
import { EmberRequestError } from '../../../src/shared/debug-client/errors';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { CPU_STATE, createEmulatorHarness, createFakeClient, STATUS, type EmulatorHarness } from '../../support/emulator-fixtures';

let harness: EmulatorHarness;

beforeEach(() => {
  harness = createEmulatorHarness();
});

/** 抛出的错误带指定错误码。 */
function hasCode(code: string): (error: FcdevError) => boolean {
  return (error) => error.code === code;
}

describe('模拟器操作：会话与参数', () => {
  it('操作表与接口版本', () => {
    assert.equal(EMULATOR_API_VERSION, 1);
    assert.deepEqual(EMULATOR_OPERATION_NAMES.toSorted(), ['breakpoint', 'connect', 'input', 'inspect', 'launch', 'load', 'record', 'run', 'screenshot', 'status', 'step']);
  });

  it('没有连接时：状态说没连，其他操作报错并提示怎么连', async () => {
    assert.deepEqual(await harness.run('status'), { connected: false });
    await assert.rejects(harness.run('run'), (error: FcdevError) => error.code === 'EMULATOR_NOT_CONNECTED' && error.hint.includes('emu_launch'));
  });

  it('未知操作与参数不是对象', async () => {
    await assert.rejects(harness.run('nothing'), (error: FcdevError) => error.code === 'EMULATOR_OPERATION_UNKNOWN' && error.hint.includes('run'));
    await assert.rejects(harness.operations.run('status', [] as never), hasCode('EMULATOR_PARAM_INVALID'));
  });

  it('连接：端口默认取设置里的调试端口，按端口找用户数据目录；握手信息与运行状态一并返回', async () => {
    harness.settings.port = 5700;
    harness.connector.next = createFakeClient();
    const result = await harness.run('connect');
    assert.equal(harness.connector.connects[0]?.userDataDirectory, path.join('fake-user-data', '5700'));
    assert.deepEqual([result.connected, result.port, result.launched], [true, 5700, false]);
    assert.deepEqual((result.runtime as { power: string }).power, 'on');
    harness.connector.connects.length = 0;
    await harness.run('connect', { port: 5701, url: 'ws://127.0.0.1:1/', token: 'abc' });
    assert.deepEqual(harness.connector.connects[0], { url: 'ws://127.0.0.1:1/', token: 'abc', userDataDirectory: path.join('fake-user-data', '5701'), clientName: 'fcdev', connectTimeoutMs: 3000 });
  });

  it('连不上时报错，并提示用 emu_launch；没有会话', async () => {
    await assert.rejects(harness.run('connect', { port: 1 }), (error: FcdevError) => error.code === 'EMULATOR_CONNECT_FAILED' && error.hint.includes('emu_launch'));
    assert.deepEqual(await harness.run('status'), { connected: false });
  });

  it('状态：握手信息、运行状态与正在进行的记录；close 关闭连接，terminate 只结束自己启动的进程', async () => {
    const client = createFakeClient({ 'inspector.traceStart': { id: 3 } });
    await harness.connect(client);
    await harness.run('record', { action: 'start', kind: 'instruction' });
    const status = await harness.run('status');
    assert.deepEqual([status.connected, status.launched, status.port, (status.runtime as { power: string }).power, status.records], [true, false, 5699, 'on', [{ id: 3, kind: 'instruction' }]]);
    assert.deepEqual(await harness.run('status', { close: true, terminate: true }), { closed: true, terminated: false });
    assert.equal(client.calls.at(-1)?.method, 'close');
    assert.deepEqual(await harness.run('status'), { connected: false });
    assert.deepEqual(harness.processes.terminated, []);
  });

  it('连接断开后会话自动清除', async () => {
    const client = createFakeClient();
    await harness.connect(client);
    client.disconnect();
    await Promise.resolve();
    assert.deepEqual(await harness.run('status'), { connected: false });
  });

  it('Ember 的请求错误换成带建议的错误', async () => {
    const error = new EmberRequestError(-32000, 'runtime.noCartridge', '没有卡带');
    await harness.connect(createFakeClient({ 'runtime.run': () => { throw error; } }));
    await assert.rejects(harness.run('run'), (caught: FcdevError) => caught.code === 'EMULATOR_REQUEST_FAILED' && caught.message.includes('runtime.noCartridge') && caught.hint.includes('载入'));
  });

  it('启动 Ember 要设置位置：没有设置或文件不存在时报错并说明设置项', async () => {
    await assert.rejects(harness.run('launch'), (error: FcdevError) => error.code === 'EMULATOR_PATH_NOT_SET' && error.hint.includes('fcdev.emulator.path'));
    harness.settings.path = path.join('no', 'such', 'Ember.exe');
    await assert.rejects(harness.run('launch'), (error: FcdevError) => error.code === 'EMULATOR_NOT_FOUND' && error.hint.includes('fcdev.emulator.path'));
  });
});

describe('模拟器操作：运行与断点', () => {
  it('运行：默认 60 帧并等结束，返回停止原因与 CPU 状态（pc 是十六进制）', async () => {
    const client = createFakeClient({ 'runtime.run': { state: { power: 'on', mode: 'paused' }, frames: 60, cycles: 1_789_773, stopped: 'completed' }, 'inspector.getState': CPU_STATE });
    await harness.connect(client);
    const result = await harness.run('run');
    assert.deepEqual(client.calls[0], { method: 'runtime.run', params: { frames: 60, wait: true } });
    assert.deepEqual([result.stopped, result.frames, result.cpu], ['completed', 60, { pc: '$C003', a: 5 }]);
    await assert.rejects(harness.run('run', { frames: 0 }), hasCode('EMULATOR_PARAM_INVALID'));
  });

  it('断点：添加、列出、删除、清空；缺参数时报错', async () => {
    const listed = { breakpoints: [{ id: 1, kind: 'execute', address: 0xc000, endAddress: 0xc000, hits: 0 }] };
    const client = createFakeClient({ 'inspector.addBreakpoint': { breakpoint: listed.breakpoints[0] }, 'inspector.listBreakpoints': listed, 'inspector.removeBreakpoint': {}, 'inspector.clearBreakpoints': { removed: 1 } });
    await harness.connect(client);
    assert.deepEqual(await harness.run('breakpoint', { action: 'add', kind: 'execute', address: '$C000' }), listed);
    assert.deepEqual(client.calls[0], { method: 'inspector.addBreakpoint', params: { kind: 'execute', address: 0xc000 } });
    await harness.run('breakpoint', { action: 'add', kind: 'write', address: 0x2000, endAddress: '$2007' });
    assert.deepEqual(client.calls[2]?.params, { kind: 'write', address: 0x2000, endAddress: 0x2007 });
    await harness.run('breakpoint', { action: 'remove', id: 1 });
    await harness.run('breakpoint', { action: 'clear' });
    assert.deepEqual(client.calls.map((call) => call.method).filter((method) => method !== 'inspector.listBreakpoints'), ['inspector.addBreakpoint', 'inspector.addBreakpoint', 'inspector.removeBreakpoint', 'inspector.clearBreakpoints']);
    await assert.rejects(harness.run('breakpoint', { action: 'add', address: 1 }), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('breakpoint', { action: 'remove' }), hasCode('EMULATOR_PARAM_INVALID'));
  });

  it('单步：默认一条指令，返回停下的 pc 与 CPU 状态', async () => {
    const client = createFakeClient({ 'runtime.step': { state: { power: 'on', mode: 'paused' }, instructions: 1, cycles: 6, frames: 0, pc: 0xc00b }, 'inspector.getState': CPU_STATE });
    await harness.connect(client);
    const result = await harness.run('step');
    assert.deepEqual(client.calls[0], { method: 'runtime.step', params: { unit: 'instruction', count: 1 } });
    assert.deepEqual([result.pc, result.instructions], ['$C00B', 1]);
  });
});

describe('模拟器操作：检查、截图与输入', () => {
  it('读内存：十六进制加字符视图，每行 16 字节', async () => {
    const data = Uint8Array.from([0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x00, 0xa9, 0x05, ...Array.from({ length: 10 }, () => 0xea)]);
    const client = createFakeClient({ 'inspector.readMemory': { space: 'cpu', address: 0xc000, length: 18, data } });
    await harness.connect(client);
    const result = await harness.run('inspect', { what: 'memory', address: '$C000', length: 18 });
    assert.deepEqual(client.calls[0]?.params, { space: 'cpu', address: 0xc000, length: 18 });
    const [first, second] = String(result.text).split('\n');
    assert.equal(first, 'C000  48 45 4C 4C 4F 00 A9 05  EA EA EA EA EA EA EA EA |HELLO...........|');
    assert.equal(second, `C010  ${'EA EA'.padEnd(48)} |..|`);
  });

  it('写内存与改寄存器：检查字节范围与 fields', async () => {
    const client = createFakeClient({ 'inspector.writeMemory': { space: 'cpu', address: 0x10, written: 2 }, 'inspector.setState': { state: CPU_STATE.states[0] } });
    await harness.connect(client);
    await harness.run('inspect', { what: 'write', address: 0x10, bytes: [1, 2] });
    assert.deepEqual(client.calls[0], { method: 'inspector.writeMemory', params: { space: 'cpu', address: 0x10, bytes: [1, 2] } });
    await harness.run('inspect', { what: 'setState', fields: [{ name: 'pc', value: 0xc000 }] });
    assert.deepEqual(client.calls[1]?.params, { module: 'cpu', fields: [{ name: 'pc', value: 0xc000 }] });
    await assert.rejects(harness.run('inspect', { what: 'write', address: 0, bytes: [256] }), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('inspect', { what: 'write', address: 0, bytes: [] }), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('inspect', { what: 'memory', address: -1 }), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('inspect'), hasCode('EMULATOR_PARAM_INVALID'));
  });

  it('反汇编：每行地址、字节与指令文字', async () => {
    const client = createFakeClient({ 'inspector.disassemble': { instructions: [{ address: 0xc000, bytes: '20 0B C0', text: 'JSR $C00B', unofficial: false }, { address: 0xc003, bytes: 'A9 05', text: 'LDA #$05', unofficial: false }] } });
    await harness.connect(client);
    const result = await harness.run('inspect', { what: 'disassemble', address: '$C000', count: 2 });
    assert.deepEqual(String(result.text).split('\n'), ['C000  20 0B C0  JSR $C00B', 'C003  A9 05     LDA #$05']);
  });

  describe('截图', () => {
    let temp: TempDir;

    beforeEach(() => { temp = createTempDir(); });
    afterEach(() => temp.cleanup());

    it('返回 base64，也可以存成文件；inline 为 false 时不返回图像', async () => {
      const png = Uint8Array.from([0x89, 0x50, 0x4e, 0x47]);
      await harness.connect(createFakeClient({ 'output.screenshot': { format: 'png', width: 256, height: 240, byteLength: 4, data: png } }));
      const inline = await harness.run('screenshot');
      assert.deepEqual([inline.width, inline.height, inline.imageBase64], [256, 240, Buffer.from(png).toString('base64')]);
      const file = path.join(temp.dir, 'shots', 'a.png');
      const saved = await harness.run('screenshot', { file, inline: false });
      assert.equal(saved.file, file);
      assert.equal(saved.imageBase64, undefined);
      assert.deepEqual([...readFileSync(file)], [...png]);
    });
  });

  it('手柄：按键名换成掩码，按下运行几帧再松开；hold 时只按下', async () => {
    const client = createFakeClient({ 'input.pad': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'inspector.getState': CPU_STATE });
    await harness.connect(client);
    await harness.run('input', { kind: 'pad', buttons: ['a', 'Start'], downFrames: 4, upFrames: 2 });
    assert.deepEqual(client.calls.filter((call) => call.method !== 'inspector.getState').map((call) => [call.method, call.params]), [
      ['input.pad', { port: 0, buttons: 9 }], ['runtime.run', { frames: 4, wait: true }], ['input.pad', { port: 0, buttons: 0 }], ['runtime.run', { frames: 2, wait: true }]
    ]);
    client.calls.length = 0;
    assert.equal((await harness.run('input', { kind: 'pad', port: 1, buttons: 128, hold: true })).held, true);
    assert.deepEqual(client.calls, [{ method: 'input.pad', params: { port: 1, buttons: 128 } }]);
    await assert.rejects(harness.run('input', { kind: 'pad', buttons: ['turbo'] }), (error: FcdevError) => error.code === 'EMULATOR_PARAM_INVALID' && error.hint.includes('select'));
  });

  it('按键与鼠标、键盘接入、全部松开', async () => {
    const client = createFakeClient({ 'input.key': {}, 'input.mouse': {}, 'input.setFamilyKeyboard': { connected: true }, 'input.releaseAll': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'inspector.getState': CPU_STATE });
    await harness.connect(client);
    await harness.run('input', { kind: 'key', code: 'Enter' });
    assert.deepEqual(client.calls.filter((call) => call.method === 'input.key').map((call) => call.params), [{ code: 'Enter', down: true }, { code: 'Enter', down: false }]);
    client.calls.length = 0;
    await harness.run('input', { kind: 'key', code: 'KeyA', action: 'down' });
    await harness.run('input', { kind: 'mouse', event: 'move', x: 10, y: 20, buttons: 1 });
    await harness.run('input', { kind: 'familyKeyboard', connected: true });
    await harness.run('input', { kind: 'releaseAll' });
    assert.deepEqual(client.calls, [
      { method: 'input.key', params: { code: 'KeyA', down: true } },
      { method: 'inspector.getState', params: { module: 'cpu' } },
      { method: 'input.mouse', params: { kind: 'move', x: 10, y: 20, buttons: 1 } },
      { method: 'input.setFamilyKeyboard', params: { connected: true } },
      { method: 'input.releaseAll', params: {} }
    ]);
  });
});

describe('模拟器操作：记录', () => {
  it('跟踪与调用记录：开始得到编号，停止得到文件；只有一个进行中的记录时可以省略 id', async () => {
    const client = createFakeClient({
      'inspector.traceStart': { id: 1 }, 'inspector.traceStop': { file: 'C:/out/trace-1.txt', lines: 10, truncated: false },
      'inspector.callLogStart': { id: 2 }, 'inspector.callLogStop': { file: 'C:/out/calllog-2.txt', lines: 3, truncated: false }
    });
    await harness.connect(client);
    assert.deepEqual(await harness.run('record', { action: 'start', kind: 'io', maxLines: 500 }), { kind: 'io', id: 1, started: true });
    assert.deepEqual(client.calls[0], { method: 'inspector.traceStart', params: { kind: 'io', maxLines: 500 } });
    assert.deepEqual(await harness.run('record', { action: 'start', kind: 'callLog', address: '$C00B', stackBytes: 2 }), { kind: 'callLog', id: 2, started: true });
    assert.deepEqual(client.calls[1], { method: 'inspector.callLogStart', params: { address: 0xc00b, stackBytes: 2 } });
    assert.deepEqual(await harness.run('record', { action: 'stop', kind: 'callLog' }), { kind: 'callLog', id: 2, stopped: true, file: 'C:/out/calllog-2.txt', lines: 3, truncated: false });
    assert.equal((await harness.run('record', { action: 'stop', kind: 'io', id: 1 })).file, 'C:/out/trace-1.txt');
    await assert.rejects(harness.run('record', { action: 'stop', kind: 'io' }), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('record', { action: 'start', kind: 'callLog' }), hasCode('EMULATOR_PARAM_INVALID'));
  });

  it('CDL：停止时先结束标记再存成文件，返回文件路径与统计', async () => {
    const client = createFakeClient({
      'inspector.cdlStart': { size: 16_384 }, 'inspector.cdlStop': { size: 16_384, codeBytes: 100, dataBytes: 20 },
      'inspector.cdlSave': { file: 'C:/out/prg.cdl', size: 16_384, codeBytes: 100, dataBytes: 20 }, 'runtime.status': STATUS
    });
    await harness.connect(client);
    assert.deepEqual(await harness.run('record', { action: 'start', kind: 'cdl' }), { kind: 'cdl', started: true, size: 16_384 });
    assert.equal((await harness.run('status')).cdl, true);
    assert.deepEqual(await harness.run('record', { action: 'stop', kind: 'cdl' }), { kind: 'cdl', stopped: true, file: 'C:/out/prg.cdl', size: 16_384, codeBytes: 100, dataBytes: 20 });
  });
});

describe('模拟器操作：载入', () => {
  it('载入：file 与 machine 二选一；文件不存在时报错', async () => {
    await harness.connect(createFakeClient());
    await assert.rejects(harness.run('load'), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('load', { file: 'a.nes', machine: 'sb2000' }), hasCode('EMULATOR_PARAM_INVALID'));
    await assert.rejects(harness.run('load', { file: path.join('no', 'such.nes') }), hasCode('EMULATOR_FILE_MISSING'));
  });

  it('载入卡带：通电但不继续运行，返回运行状态', async () => {
    const temp = createTempDir();
    try {
      const file = writeTestFile(temp.dir, 'a.nes', 'x');
      const client = createFakeClient({ 'cartridges.load': {} });
      await harness.connect(client);
      assert.equal((await harness.run('load', { file })).power, 'on');
      assert.deepEqual(client.calls[0], { method: 'cartridges.load', params: { path: file, powerOn: true, resume: false } });
    } finally {
      temp.cleanup();
    }
  });
});
