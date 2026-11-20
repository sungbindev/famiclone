// ------------------------------------------------------------------------
// 名称：emulator-operations.test.mjs
// 说明：测试 Copilot 模拟器工具对应的操作：会话、运行、断点、单步、检查、截图、输入、记录；以及在真实 Ember 里“记录、导入、再追踪”的闭环。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：前面的用例用假的调试客户端（只记录请求、按方法返回预设的结果），不启动 Ember；真实 Ember 的用例需要 FCDEV_EMULATOR、cc65 与已构建的调试 SDK，缺一就跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, readFileSync, rmSync } from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { runOperation } from '../src/disasm/index.mjs';
import { EMULATOR_API_VERSION, EMULATOR_OPERATION_NAMES, debugUserDataDirectory, locateEmulator, resolveDebugCli, runEmulatorOperation } from '../src/emulator/index.mjs';
import { closeSession, openSession } from '../src/emulator/operations/session.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 默认的 CPU 状态 */
const CPU_STATE = { states: [{ module: 'cpu', writable: true, fields: [{ name: 'pc', value: 0xc003 }, { name: 'a', value: 5 }] }] };

/**
 * 假的调试客户端：记录请求，按方法返回预设的结果；没有预设的方法抛错。
 * @param {Record<string, unknown>} handlers 方法名到结果或函数
 * @returns {{calls: Array<{method: string, params: unknown}>, request: Function, close: Function, closed: Promise<void>, info: object}} 客户端
 */
function createFakeClient(handlers) {
  const calls = [];
  return {
    calls,
    info: { appVersion: 'test' },
    closed: new Promise(() => {}),
    close() {
      calls.push({ method: 'close', params: null });
    },
    async request(method, params) {
      calls.push({ method, params });
      const handler = handlers[method];
      if (handler === undefined) throw new Error(`没有预设的请求 ${method}`);
      return typeof handler === 'function' ? handler(params) : handler;
    },
  };
}

/** 运行状态的预设结果 */
const STATUS = { power: 'on', mode: 'paused', frame: 0, cycles: 0, speed: 100, mouse: false };

/** 每个用例结束时关掉会话，免得串到下一个用例 */
afterEach(() => closeSession());

describe('模拟器操作：会话与参数', () => {
  it('操作表与接口版本', () => {
    assert.equal(EMULATOR_API_VERSION, 1);
    assert.deepEqual([...EMULATOR_OPERATION_NAMES].sort(), ['breakpoint', 'connect', 'input', 'inspect', 'launch', 'load', 'record', 'run', 'screenshot', 'status', 'step']);
  });

  it('没有连接时：状态说没连，其他操作报错并提示怎么连', async () => {
    assert.deepEqual(await runEmulatorOperation('status', {}), { connected: false });
    await assert.rejects(runEmulatorOperation('run', {}), (error) => error.code === 'EMULATOR_NOT_CONNECTED' && error.hint.includes('emu_launch'));
  });

  it('未知操作与参数不是对象', async () => {
    await assert.rejects(runEmulatorOperation('nothing', {}), (error) => error.code === 'EMULATOR_OPERATION_UNKNOWN' && error.hint.includes('run'));
    await assert.rejects(runEmulatorOperation('status', []), (error) => error.code === 'EMULATOR_PARAM_INVALID');
  });

  it('状态：握手信息、运行状态与正在进行的记录；close 关闭连接，terminate 只结束自己启动的进程', async () => {
    const client = createFakeClient({ 'runtime.status': STATUS, 'inspector.traceStart': { id: 3 } });
    openSession({ client, port: 5699 });
    await runEmulatorOperation('record', { action: 'start', kind: 'instruction' });
    const status = await runEmulatorOperation('status', {});
    assert.deepEqual([status.connected, status.launched, status.port, status.runtime.power, status.records], [true, false, 5699, 'on', [{ id: 3, kind: 'instruction' }]]);
    assert.deepEqual(await runEmulatorOperation('status', { close: true, terminate: true }), { closed: true, terminated: false });
    assert.equal(client.calls.at(-1).method, 'close');
    assert.deepEqual(await runEmulatorOperation('status', {}), { connected: false });
  });

  it('Ember 的请求错误换成带建议的错误', async () => {
    const error = Object.assign(new Error('没有卡带'), { name: 'EmberRequestError', code: 'runtime.noCartridge' });
    openSession({ client: createFakeClient({ 'runtime.run': () => { throw error; } }) });
    await assert.rejects(runEmulatorOperation('run', {}), (caught) => caught.code === 'EMULATOR_REQUEST_FAILED' && caught.message.includes('runtime.noCartridge') && caught.hint.includes('载入'));
  });
});

describe('模拟器操作：运行与断点', () => {
  it('运行：默认 60 帧并等结束，返回停止原因与 CPU 状态（pc 是十六进制）', async () => {
    const client = createFakeClient({ 'runtime.run': { state: { power: 'on', mode: 'paused' }, frames: 60, cycles: 1789773, stopped: 'completed' }, 'inspector.getState': CPU_STATE });
    openSession({ client });
    const result = await runEmulatorOperation('run', {});
    assert.deepEqual(client.calls[0], { method: 'runtime.run', params: { frames: 60, wait: true } });
    assert.deepEqual([result.stopped, result.frames, result.cpu], ['completed', 60, { pc: '$C003', a: 5 }]);
    await assert.rejects(runEmulatorOperation('run', { frames: 0 }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
  });

  it('断点：添加、列出、删除、清空；缺参数时报错', async () => {
    const listed = { breakpoints: [{ id: 1, kind: 'execute', address: 0xc000, endAddress: 0xc000, hits: 0 }] };
    const client = createFakeClient({ 'inspector.addBreakpoint': { breakpoint: listed.breakpoints[0] }, 'inspector.listBreakpoints': listed, 'inspector.removeBreakpoint': {}, 'inspector.clearBreakpoints': { removed: 1 } });
    openSession({ client });
    assert.deepEqual(await runEmulatorOperation('breakpoint', { action: 'add', kind: 'execute', address: '$C000' }), listed);
    assert.deepEqual(client.calls[0], { method: 'inspector.addBreakpoint', params: { kind: 'execute', address: 0xc000 } });
    await runEmulatorOperation('breakpoint', { action: 'add', kind: 'write', address: 0x2000, endAddress: '$2007' });
    assert.deepEqual(client.calls[2].params, { kind: 'write', address: 0x2000, endAddress: 0x2007 });
    await runEmulatorOperation('breakpoint', { action: 'remove', id: 1 });
    await runEmulatorOperation('breakpoint', { action: 'clear' });
    assert.deepEqual(client.calls.map((call) => call.method).filter((method) => method !== 'inspector.listBreakpoints'), [
      'inspector.addBreakpoint', 'inspector.addBreakpoint', 'inspector.removeBreakpoint', 'inspector.clearBreakpoints',
    ]);
    await assert.rejects(runEmulatorOperation('breakpoint', { action: 'add', address: 1 }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('breakpoint', { action: 'remove' }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
  });

  it('单步：默认一条指令，返回停下的 pc 与 CPU 状态', async () => {
    const client = createFakeClient({ 'runtime.step': { state: { power: 'on', mode: 'paused' }, instructions: 1, cycles: 6, frames: 0, pc: 0xc00b }, 'inspector.getState': CPU_STATE });
    openSession({ client });
    const result = await runEmulatorOperation('step', {});
    assert.deepEqual(client.calls[0], { method: 'runtime.step', params: { unit: 'instruction', count: 1 } });
    assert.deepEqual([result.pc, result.instructions], ['$C00B', 1]);
  });
});

describe('模拟器操作：检查、截图与输入', () => {
  it('读内存：十六进制加字符视图，每行 16 字节', async () => {
    const data = Uint8Array.from([0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x00, 0xa9, 0x05, ...new Array(10).fill(0xea)]);
    const client = createFakeClient({ 'inspector.readMemory': { space: 'cpu', address: 0xc000, length: 18, data } });
    openSession({ client });
    const result = await runEmulatorOperation('inspect', { what: 'memory', address: '$C000', length: 18 });
    assert.deepEqual(client.calls[0].params, { space: 'cpu', address: 0xc000, length: 18 });
    const [first, second] = result.text.split('\n');
    assert.equal(first, 'C000  48 45 4C 4C 4F 00 A9 05  EA EA EA EA EA EA EA EA |HELLO...........|');
    assert.equal(second, `C010  ${'EA EA'.padEnd(48)} |..|`);
  });

  it('写内存与改寄存器：检查字节范围与 fields', async () => {
    const client = createFakeClient({ 'inspector.writeMemory': { space: 'cpu', address: 0x10, written: 2 }, 'inspector.setState': { state: CPU_STATE.states[0] } });
    openSession({ client });
    await runEmulatorOperation('inspect', { what: 'write', address: 0x10, bytes: [1, 2] });
    assert.deepEqual(client.calls[0], { method: 'inspector.writeMemory', params: { space: 'cpu', address: 0x10, bytes: [1, 2] } });
    await runEmulatorOperation('inspect', { what: 'setState', fields: [{ name: 'pc', value: 0xc000 }] });
    assert.deepEqual(client.calls[1].params, { module: 'cpu', fields: [{ name: 'pc', value: 0xc000 }] });
    await assert.rejects(runEmulatorOperation('inspect', { what: 'write', address: 0, bytes: [256] }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('inspect', { what: 'write', address: 0, bytes: [] }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('inspect', { what: 'memory', address: -1 }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('inspect', {}), (error) => error.code === 'EMULATOR_PARAM_INVALID');
  });

  it('反汇编：每行地址、字节与指令文字', async () => {
    const client = createFakeClient({ 'inspector.disassemble': { instructions: [{ address: 0xc000, bytes: '20 0B C0', text: 'JSR $C00B', unofficial: false }, { address: 0xc003, bytes: 'A9 05', text: 'LDA #$05', unofficial: false }] } });
    openSession({ client });
    const result = await runEmulatorOperation('inspect', { what: 'disassemble', address: '$C000', count: 2 });
    assert.deepEqual(result.text.split('\n'), ['C000  20 0B C0  JSR $C00B', 'C003  A9 05     LDA #$05']);
  });

  describe('截图', () => {
    let temp;

    beforeEach(() => { temp = createTempDir(); });
    afterEach(() => temp.cleanup());

    it('返回 base64，也可以存成文件；inline 为 false 时不返回图像', async () => {
      const png = Uint8Array.from([0x89, 0x50, 0x4e, 0x47]);
      openSession({ client: createFakeClient({ 'output.screenshot': { format: 'png', width: 256, height: 240, byteLength: 4, data: png } }) });
      const inline = await runEmulatorOperation('screenshot', {});
      assert.deepEqual([inline.width, inline.height, inline.imageBase64], [256, 240, Buffer.from(png).toString('base64')]);
      const file = path.join(temp.dir, 'shots', 'a.png');
      const saved = await runEmulatorOperation('screenshot', { file, inline: false });
      assert.equal(saved.file, file);
      assert.equal(saved.imageBase64, undefined);
      assert.deepEqual([...readFileSync(file)], [...png]);
    });
  });

  it('手柄：按键名换成掩码，按下运行几帧再松开；hold 时只按下', async () => {
    const client = createFakeClient({ 'input.pad': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'inspector.getState': CPU_STATE });
    openSession({ client });
    await runEmulatorOperation('input', { kind: 'pad', buttons: ['a', 'Start'], downFrames: 4, upFrames: 2 });
    assert.deepEqual(client.calls.filter((call) => call.method !== 'inspector.getState').map((call) => [call.method, call.params]), [
      ['input.pad', { port: 0, buttons: 9 }], ['runtime.run', { frames: 4, wait: true }], ['input.pad', { port: 0, buttons: 0 }], ['runtime.run', { frames: 2, wait: true }],
    ]);
    client.calls.length = 0;
    assert.equal((await runEmulatorOperation('input', { kind: 'pad', port: 1, buttons: 128, hold: true })).held, true);
    assert.deepEqual(client.calls, [{ method: 'input.pad', params: { port: 1, buttons: 128 } }]);
    await assert.rejects(runEmulatorOperation('input', { kind: 'pad', buttons: ['turbo'] }), (error) => error.code === 'EMULATOR_PARAM_INVALID' && error.hint.includes('select'));
  });

  it('按键与鼠标、键盘接入、全部松开', async () => {
    const client = createFakeClient({ 'input.key': {}, 'input.mouse': {}, 'input.setFamilyKeyboard': { connected: true }, 'input.releaseAll': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'inspector.getState': CPU_STATE });
    openSession({ client });
    await runEmulatorOperation('input', { kind: 'key', code: 'Enter' });
    assert.deepEqual(client.calls.filter((call) => call.method === 'input.key').map((call) => call.params), [{ code: 'Enter', down: true }, { code: 'Enter', down: false }]);
    client.calls.length = 0;
    await runEmulatorOperation('input', { kind: 'key', code: 'KeyA', action: 'down' });
    await runEmulatorOperation('input', { kind: 'mouse', event: 'move', x: 10, y: 20, buttons: 1 });
    await runEmulatorOperation('input', { kind: 'familyKeyboard', connected: true });
    await runEmulatorOperation('input', { kind: 'releaseAll' });
    assert.deepEqual(client.calls, [
      { method: 'input.key', params: { code: 'KeyA', down: true } },
      { method: 'inspector.getState', params: { module: 'cpu' } },
      { method: 'input.mouse', params: { kind: 'move', x: 10, y: 20, buttons: 1 } },
      { method: 'input.setFamilyKeyboard', params: { connected: true } },
      { method: 'input.releaseAll', params: {} },
    ]);
  });
});

describe('模拟器操作：记录', () => {
  it('跟踪与调用记录：开始得到编号，停止得到文件；只有一个进行中的记录时可以省略 id', async () => {
    const client = createFakeClient({
      'inspector.traceStart': { id: 1 }, 'inspector.traceStop': { file: 'C:/out/trace-1.txt', lines: 10, truncated: false },
      'inspector.callLogStart': { id: 2 }, 'inspector.callLogStop': { file: 'C:/out/calllog-2.txt', lines: 3, truncated: false },
    });
    openSession({ client });
    assert.deepEqual(await runEmulatorOperation('record', { action: 'start', kind: 'io', maxLines: 500 }), { kind: 'io', id: 1, started: true });
    assert.deepEqual(client.calls[0], { method: 'inspector.traceStart', params: { kind: 'io', maxLines: 500 } });
    assert.deepEqual(await runEmulatorOperation('record', { action: 'start', kind: 'callLog', address: '$C00B', stackBytes: 2 }), { kind: 'callLog', id: 2, started: true });
    assert.deepEqual(client.calls[1], { method: 'inspector.callLogStart', params: { address: 0xc00b, stackBytes: 2 } });
    assert.deepEqual(await runEmulatorOperation('record', { action: 'stop', kind: 'callLog' }), { kind: 'callLog', id: 2, stopped: true, file: 'C:/out/calllog-2.txt', lines: 3, truncated: false });
    assert.equal((await runEmulatorOperation('record', { action: 'stop', kind: 'io', id: 1 })).file, 'C:/out/trace-1.txt');
    await assert.rejects(runEmulatorOperation('record', { action: 'stop', kind: 'io' }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('record', { action: 'start', kind: 'callLog' }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
  });

  it('CDL：停止时先结束标记再存成文件，返回文件路径与统计', async () => {
    const client = createFakeClient({
      'inspector.cdlStart': { size: 16384 }, 'inspector.cdlStop': { size: 16384, codeBytes: 100, dataBytes: 20 },
      'inspector.cdlSave': { file: 'C:/out/prg.cdl', size: 16384, codeBytes: 100, dataBytes: 20 }, 'runtime.status': STATUS,
    });
    openSession({ client });
    assert.deepEqual(await runEmulatorOperation('record', { action: 'start', kind: 'cdl' }), { kind: 'cdl', started: true, size: 16384 });
    assert.equal((await runEmulatorOperation('status', {})).cdl, true);
    assert.deepEqual(await runEmulatorOperation('record', { action: 'stop', kind: 'cdl' }), { kind: 'cdl', stopped: true, file: 'C:/out/prg.cdl', size: 16384, codeBytes: 100, dataBytes: 20 });
  });
});

describe('模拟器操作：载入', () => {
  it('载入：file 与 machine 二选一；文件不存在时报错', async () => {
    openSession({ client: createFakeClient({}) });
    await assert.rejects(runEmulatorOperation('load', {}), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('load', { file: 'a.nes', machine: 'sb2000' }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('load', { file: path.join('no', 'such.nes') }), (error) => error.code === 'EMULATOR_FILE_MISSING');
  });

  it('载入卡带：通电但不继续运行，返回运行状态', async () => {
    const temp = createTempDir();
    try {
      const file = writeTestFile(temp.dir, 'a.nes', 'x');
      const client = createFakeClient({ 'cartridges.load': {}, 'runtime.status': STATUS });
      openSession({ client });
      assert.equal((await runEmulatorOperation('load', { file })).power, 'on');
      assert.deepEqual(client.calls[0], { method: 'cartridges.load', params: { path: file, powerOn: true, resume: false } });
    } finally {
      temp.cleanup();
    }
  });
});

/** 调试 SDK 是否已构建 */
function isSdkBuilt() {
  try {
    resolveDebugCli();
    return true;
  } catch {
    return false;
  }
}

/** 在随机空闲端口上监听后释放，返回这个端口 */
async function findFreePort() {
  const server = net.createServer();
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const { port } = server.address();
  await new Promise((resolve) => server.close(resolve));
  return port;
}

/** 找真实的 Ember（需要环境变量 FCDEV_EMULATOR）；没有时返回 null */
function findRealEmber() {
  if (process.env.FCDEV_SKIP_GUI_TESTS === '1' || !process.env.FCDEV_EMULATOR) return null;
  try {
    return locateEmulator();
  } catch {
    return null;
  }
}

const SKIP_WITHOUT_SDK = isSdkBuilt() ? false : '调试 SDK 没有构建（在 packages/fcdev 目录里先运行 npm run debug-sdk:build）';

describe('模拟器操作：连接（需要调试 SDK）', { skip: SKIP_WITHOUT_SDK }, () => {
  it('没有运行的 Ember 时连接失败，并提示用 emu_launch', async () => {
    await assert.rejects(runEmulatorOperation('connect', { port: 1 }), (error) => error.code === 'EMULATOR_CONNECT_FAILED' && error.hint.includes('emu_launch'));
    assert.deepEqual(await runEmulatorOperation('status', {}), { connected: false });
  });
});

const realEmber = findRealEmber();

describe('真实 Ember：记录、导入、再追踪', { skip: realEmber && !SKIP_WITHOUT_TOOLCHAIN && !SKIP_WITHOUT_SDK ? false : '没有 Ember（FCDEV_EMULATOR）、cc65 工具链或调试 SDK，或设置了 FCDEV_SKIP_GUI_TESTS' }, () => {
  it('启动并载入 NES 程序，记录 CDL 后导入反汇编工程，再追踪得到代码', async () => {
    const temp = createTempDir();
    const port = await findFreePort();
    try {
      writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
      writeTestFile(temp.dir, 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "TEST"); nes_wait_vblank(); nes_screen_on(); for (;;) { } }\n');
      const { result } = await buildPath({ pathArg: 'hello.c', cwd: temp.dir });

      const launched = await runEmulatorOperation('launch', { port });
      assert.equal(launched.runtime.power, 'off');
      const loaded = await runEmulatorOperation('load', { file: result.outputFile });
      assert.equal(loaded.power, 'on');
      await runEmulatorOperation('record', { action: 'start', kind: 'cdl' });
      assert.equal((await runEmulatorOperation('run', { frames: 30 })).stopped, 'completed');
      const cdl = await runEmulatorOperation('record', { action: 'stop', kind: 'cdl' });
      assert.ok(cdl.codeBytes > 0 && existsSync(cdl.file), `CDL 没有记到代码：${JSON.stringify(cdl)}`);

      const projectDir = path.join(temp.dir, 'project');
      await runOperation('open', { projectDir, image: result.outputFile });
      const imported = await runOperation('importRecord', { projectDir, kind: 'cdl', file: cdl.file });
      assert.ok(imported.entriesAdded > 0, JSON.stringify(imported));
      assert.ok((await runOperation('trace', { projectDir })).instructions > 0);
      assert.ok((await runEmulatorOperation('screenshot', {})).imageBase64.startsWith('iVBOR'));
    } finally {
      await runEmulatorOperation('status', { close: true, terminate: true });
      try {
        rmSync(debugUserDataDirectory(port), { recursive: true, force: true, maxRetries: 10, retryDelay: 300 });
      } catch {
        // Ember 退出需要一点时间，目录还被占用时放弃清理也不影响测试结果
      }
      temp.cleanup();
    }
  });
});
