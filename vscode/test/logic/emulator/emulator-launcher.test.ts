// ------------------------------------------------------------------------
// 名称：emulator-launcher.test.ts
// 说明：测试启动 Ember：自由运行的命令行、调试模式的启动参数与载入、学习机的整机开机与输入程序名、
//       载入失败与等待超时的收尾。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用假的进程启动与假的调试连接，不启动 Ember。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { DebugSession } from '../../../src/contracts/debug-client/debug-session';
import type { ConnectToEmberOptions } from '../../../src/contracts/debug-client/debug-connector';
import { launchEmulator } from '../../../src/logic/emulator/emulator-launcher';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { createFakeClient, FakeConnector, FakeProcesses } from '../../support/emulator-fixtures';

/** 前几次连接失败、之后成功的连接：模拟 Ember 还没写出端点文件。 */
class LateConnector extends FakeConnector {
  attempts = 0;

  /** @param failures 失败的次数。 */
  constructor(private readonly failures: number) {
    super();
  }

  override async connectToEmber(options: ConnectToEmberOptions = {}): Promise<DebugSession> {
    this.attempts += 1;
    if (this.attempts <= this.failures) throw new Error('端点文件还没写出');
    return super.connectToEmber(options);
  }
}

describe('启动 Ember', () => {
  const files = new NodeFileSystem();
  let temp: TempDir;
  let processes: FakeProcesses;
  let connector: FakeConnector;
  let rom: string;
  let firmware: string;
  let disk: string;

  beforeEach(() => {
    temp = createTempDir();
    processes = new FakeProcesses();
    connector = new FakeConnector();
    rom = writeTestFile(temp.dir, 'game.nes', 'x');
    firmware = writeTestFile(temp.dir, 'bios.nes', 'x');
    disk = writeTestFile(temp.dir, 'disk.img', 'x');
  });
  afterEach(() => temp.cleanup());

  it('自由运行：用 --cartridge 打开窗口，进程退出不算失败，不连调试服务', async () => {
    const result = await launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom });
    assert.deepEqual(processes.started, [{ executable: 'Ember.exe', args: [`--cartridge=${rom}`], failOnExit: false }]);
    assert.deepEqual([result.pid, result.client], [4242, null]);
    assert.equal(connector.connects.length, 0);
    assert.deepEqual(processes.checkedPorts, []);
  });

  it('要加载的文件不存在时在启动之前报错', async () => {
    await assert.rejects(launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: 'none.nes' }), (error: FcdevError) => error.code === 'EMULATOR_FILE_MISSING');
    assert.equal(processes.started.length, 0);
  });

  it('自由运行起不来时报启动失败', async () => {
    processes.startError = new FcdevError('spawn error', { code: 'EMULATOR_START_FAILED' });
    await assert.rejects(launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom }), /spawn error/);
  });

  it('调试模式：先检查端口，用独立的用户数据目录、调试端口与令牌启动，连上后载入卡带并只上电', async () => {
    const client = createFakeClient({ 'cartridges.load': {} });
    connector.next = client;
    const result = await launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom, debug: true, port: 5688 });
    assert.deepEqual(processes.checkedPorts, [5688]);
    const userData = processes.userDataDirectory(5688);
    assert.deepEqual(processes.started, [{ executable: 'Ember.exe', args: [`--user-data-dir=${userData}`, '--debug-port=5688', '--debug-token=test-token'], failOnExit: true }]);
    assert.deepEqual(connector.connects[0], { userDataDirectory: userData, clientName: 'fcdev', connectTimeoutMs: 1000 });
    assert.deepEqual(client.calls, [{ method: 'cartridges.load', params: { path: rom, powerOn: true, resume: false } }]);
    assert.equal(result.client, client);
  });

  it('端口被占用时不启动进程', async () => {
    processes.portError = new Error('占用');
    await assert.rejects(launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom, debug: true }), /占用/);
    assert.equal(processes.started.length, 0);
  });

  it('不给要加载的内容的调试模式：只启动并连接', async () => {
    const client = createFakeClient();
    connector.next = client;
    const result = await launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', debug: true });
    assert.deepEqual(client.calls, []);
    assert.equal(result.client, client);
  });

  it('学习机的调试模式：添加固件、开机、放入磁盘，停在开机状态，不输入程序名', async () => {
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {} });
    connector.next = client;
    await launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', sb2000: { firmware, diskImage: disk, programName: 'HELLO' }, debug: true });
    assert.deepEqual(client.calls.map((call) => call.method), ['machines.setRom', 'machines.boot', 'media.floppyInsert']);
  });

  it('学习机的自由运行：开机后在 SB-DOS 里输入程序名，继续运行并断开连接', async () => {
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {}, 'runtime.resume': {} });
    connector.next = client;
    const result = await launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', sb2000: { firmware, diskImage: disk, programName: 'Hi' } });
    const downs = client.calls.filter((call) => call.method === 'input.key' && (call.params as { down: boolean }).down).map((call) => (call.params as { code: string }).code);
    assert.deepEqual(downs, ['Enter', 'Enter', 'KeyH', 'KeyI', 'Enter']);
    assert.deepEqual(client.calls.filter((call) => call.method === 'runtime.run')[0]?.params, { frames: 900, wait: true });
    assert.deepEqual(client.calls.slice(-2).map((call) => call.method), ['runtime.resume', 'close']);
    assert.equal(result.client, null);
  });

  it('载入被拒绝时关闭连接、结束进程，并说明无法载入', async () => {
    const client = createFakeClient({ 'cartridges.load': () => { throw new Error('不是卡带'); } });
    connector.next = client;
    await assert.rejects(launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom, debug: true }), (error: FcdevError) => error.code === 'EMULATOR_LOAD_FAILED' && error.message.includes('不是卡带'));
    assert.equal(client.calls.at(-1)?.method, 'close');
    assert.equal(processes.kills, 1);
  });

  it('调试服务还没就绪时稍后重试，就绪后载入', async () => {
    const late = new LateConnector(2);
    late.next = createFakeClient({ 'cartridges.load': {} });
    const result = await launchEmulator({ files, connector: late, processes }, { executable: 'Ember.exe', romFile: rom, debug: true });
    assert.equal(late.attempts, 3);
    assert.notEqual(result.client, null);
  });

  it('一直连不上时超时报错并结束进程', async () => {
    await assert.rejects(launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom, debug: true, readyTimeoutMs: 50 }), (error: FcdevError) => error.code === 'EMULATOR_NOT_READY');
    assert.equal(processes.kills, 1);
  });

  it('进程提前退出时直接报启动失败，不再等到超时', async () => {
    processes.startError = new FcdevError('Ember 退出了', { code: 'EMULATOR_START_FAILED' });
    await assert.rejects(launchEmulator({ files, connector, processes }, { executable: 'Ember.exe', romFile: rom, debug: true }), /Ember 退出了/);
  });
});
