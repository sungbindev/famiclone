// ------------------------------------------------------------------------
// 名称：emulator-program-runner.test.ts
// 说明：测试构建程序后在 Ember 里运行：目标没有运行方案、整机目标没有固件、没有设置 Ember 位置时在
//       构建之前报错；NES 与学习机运行时的启动内容；调试运行的端口取自设置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：构建服务、进程启动与调试连接都是替身，不运行 cc65，也不启动 Ember。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { BuildOutcome, BuildService } from '../../../src/contracts/build/build-service';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { EmulatorProgramRunner } from '../../../src/logic/emulator/emulator-program-runner';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { createFakeClient, FakeConnector, FakeProcesses, FakeSettings } from '../../support/emulator-fixtures';

/**
 * 假的构建服务：计划里只有运行方案、固件、目标专用选项与磁盘镜像，
 * 记录是否构建、是否另外生成镜像。
 */
class FakeBuildService {
  builds = 0;
  diskBuilds = 0;
  /** 目标专用选项。 */
  targetOptions: Record<string, unknown> = {};
  /** 项目配置里的 diskImage。 */
  diskImage: string | undefined;

  /**
   * @param mode 运行方案；null 表示目标没有运行方案。 @param firmware 项目配置里的固件。 @param
   * output 产物路径。 @param machine 整机机型。 @param builtDisk 构建时是否已经生成了镜像。
   */
  constructor(
    private readonly mode: 'rom' | 'firmware-disk' | 'firmware' | null,
    private readonly firmware: string | undefined,
    private readonly output: string,
    private readonly machine: string = 'sb2000',
    private readonly builtDisk = false
  ) {}

  plan(): ReturnType<BuildService['plan']> {
    const run = this.mode === null ? null : { emulator: 'ember', mode: this.mode, machine: this.mode === 'firmware-disk' ? this.machine : null };
    return { target: { name: 'fake', run }, config: { firmware: this.firmware, diskImage: this.diskImage }, targetOptions: this.targetOptions } as never;
  }

  async build(): Promise<BuildOutcome> {
    this.builds += 1;
    const disk = this.builtDisk ? { imageFile: this.output.replace(/\.exe$/, '.img'), size: 1, files: ['HELLO.EXE'] } : undefined;
    return { plan: this.plan(), result: { programName: 'hello', outputFile: this.output, size: 1, compiled: 1, reused: 0 }, disk };
  }

  buildDisk(): ReturnType<BuildService['buildDisk']> {
    this.diskBuilds += 1;
    return { imageFile: this.output.replace(/\.exe$/, '.img'), size: 1, files: ['HELLO.EXE'] };
  }
}

describe('构建并运行', () => {
  let temp: TempDir;
  let processes: FakeProcesses;
  let connector: FakeConnector;
  let settings: FakeSettings;
  let ember: string;

  beforeEach(() => {
    temp = createTempDir();
    processes = new FakeProcesses();
    connector = new FakeConnector();
    ember = writeTestFile(temp.dir, 'Ember.exe', 'x');
    settings = new FakeSettings(ember, 5701);
  });
  afterEach(() => temp.cleanup());

  /** 组装运行器：文件系统是真的，其余是替身。 */
  function createRunner(build: FakeBuildService): EmulatorProgramRunner {
    return new EmulatorProgramRunner(build as unknown as BuildService, { files: new NodeFileSystem(), profiles: {} as never, connector, settings, processes });
  }

  it('目标没有运行方案时在构建之前报错', async () => {
    const build = new FakeBuildService(null, undefined, 'x');
    await assert.rejects(createRunner(build).run({ build: { cwd: temp.dir }, debug: false }), (error: FcdevError) => error.code === 'TARGET_NO_RUN');
    assert.equal(build.builds, 0);
  });

  it('整机目标没有固件时在构建之前报错，并提示设置 firmware', async () => {
    const build = new FakeBuildService('firmware-disk', undefined, 'x');
    await assert.rejects(createRunner(build).run({ build: { cwd: temp.dir }, debug: false }), (error: FcdevError) => error.code === 'FIRMWARE_MISSING' && error.hint.includes('firmware'));
    assert.equal(build.builds, 0);
  });

  it('没有设置 Ember 的位置时在构建之前报错，并说明设置项', async () => {
    settings.path = undefined;
    const build = new FakeBuildService('rom', undefined, 'x');
    await assert.rejects(createRunner(build).run({ build: { cwd: temp.dir }, debug: false }), (error: FcdevError) => error.code === 'EMULATOR_PATH_NOT_SET' && error.hint.includes('fcdev.emulator.path'));
    assert.equal(build.builds, 0);
  });

  it('NES 程序：构建后用产物自由运行', async () => {
    const rom = writeTestFile(temp.dir, 'hello.nes', 'x');
    const result = await createRunner(new FakeBuildService('rom', undefined, rom)).run({ build: { cwd: temp.dir }, debug: false });
    assert.deepEqual(processes.started, [{ executable: ember, args: [`--cartridge=${rom}`], failOnExit: false }]);
    assert.deepEqual(result, { pid: 4242, description: rom, debug: false, port: undefined, firmwareDisk: false });
  });

  it('NES 程序调试运行：端口取自设置，载入后断开连接，Ember 保持运行', async () => {
    const rom = writeTestFile(temp.dir, 'hello.nes', 'x');
    const client = createFakeClient({ 'cartridges.load': {} });
    connector.next = client;
    const result = await createRunner(new FakeBuildService('rom', undefined, rom)).run({ build: { cwd: temp.dir }, debug: true });
    assert.deepEqual(processes.checkedPorts, [5701]);
    assert.deepEqual([result.debug, result.port, result.firmwareDisk], [true, 5701, false]);
    assert.equal(client.calls.at(-1)?.method, 'close');
    assert.equal(processes.kills, 0);
  });

  it('学习机程序：放进磁盘镜像，加载项目里的固件，自由运行时输入程序名', async () => {
    const exe = writeTestFile(temp.dir, 'hello.exe', 'x');
    writeTestFile(temp.dir, 'hello.img', 'x');
    const firmware = writeTestFile(temp.dir, 'bios.nes', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {}, 'runtime.resume': {} });
    connector.next = client;
    const result = await createRunner(new FakeBuildService('firmware-disk', firmware, exe)).run({ build: { cwd: temp.dir }, debug: false });
    assert.deepEqual(client.calls[0], { method: 'machines.setRom', params: { machine: 'sb2000', path: firmware } });
    assert.match(result.description, /hello\.img（程序 hello）/);
    assert.equal(result.firmwareDisk, true);
    const downs = client.calls.filter((call) => call.method === 'input.key' && (call.params as { down: boolean }).down).length;
    assert.equal(downs, 'hello'.length + 3);
  });

  it('SB97 程序：按目标的机型开机，先选中 DOS 再输入程序名；构建已生成镜像时不再另外生成', async () => {
    const exe = writeTestFile(temp.dir, 'hello.exe', 'x');
    writeTestFile(temp.dir, 'hello.img', 'x');
    const firmware = writeTestFile(temp.dir, 'sb97.bin', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {}, 'runtime.resume': {} });
    connector.next = client;
    const build = new FakeBuildService('firmware-disk', firmware, exe, 'sb97', true);
    await createRunner(build).run({ build: { cwd: temp.dir }, debug: false });
    assert.deepEqual(client.calls[0], { method: 'machines.setRom', params: { machine: 'sb97', path: firmware } });
    const keys = client.calls.filter((call) => call.method === 'input.key' && (call.params as { down: boolean }).down).map((call) => (call.params as { code: string }).code);
    assert.deepEqual(keys, ['Space', 'ArrowLeft', 'Enter', 'KeyH', 'KeyE', 'KeyL', 'KeyL', 'KeyO', 'Enter']);
    assert.equal(build.diskBuilds, 0);
  });

  it('裕兴程序：经主菜单与“学DOS”进入 DOS，输入带 .EXE 扩展名的程序名', async () => {
    const exe = writeTestFile(temp.dir, 'hello.exe', 'x');
    writeTestFile(temp.dir, 'hello.img', 'x');
    const firmware = writeTestFile(temp.dir, 'yuxing.bin', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {}, 'runtime.resume': {} });
    connector.next = client;
    await createRunner(new FakeBuildService('firmware-disk', firmware, exe, 'yuxing', true)).run({ build: { cwd: temp.dir }, debug: false });
    assert.deepEqual(client.calls[0], { method: 'machines.setRom', params: { machine: 'yuxing', path: firmware } });
    const keys = client.calls.filter((call) => call.method === 'input.key' && (call.params as { down: boolean }).down).map((call) => (call.params as { code: string }).code);
    assert.deepEqual(keys, [
      'Space', 'ArrowDown', 'ArrowDown', 'ArrowDown', 'Enter', 'Space', 'ArrowDown', 'ArrowDown', 'Enter',
      'KeyH', 'KeyE', 'KeyL', 'KeyL', 'KeyO', 'Period', 'KeyE', 'KeyX', 'KeyE', 'Enter'
    ]);
  });

  it('步步高程序：等系统盘启动后按两次回车跳过日期与时间，输入不带扩展名的程序名', async () => {
    const exe = writeTestFile(temp.dir, 'hello.cmd', 'x');
    writeTestFile(temp.dir, 'hello.img', 'x');
    const firmware = writeTestFile(temp.dir, 'bbg1_bios.bin', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {}, 'runtime.resume': {} });
    connector.next = client;
    await createRunner(new FakeBuildService('firmware-disk', firmware, exe, 'bbg1', true)).run({ build: { cwd: temp.dir }, debug: false });
    assert.deepEqual(client.calls[0], { method: 'machines.setRom', params: { machine: 'bbg1', path: firmware } });
    const keys = client.calls.filter((call) => call.method === 'input.key' && (call.params as { down: boolean }).down).map((call) => (call.params as { code: string }).code);
    assert.deepEqual(keys, ['Enter', 'Enter', 'KeyH', 'KeyE', 'KeyL', 'KeyL', 'KeyO', 'Enter']);
  });

  it('构建没有生成镜像时（没有配置 diskName），运行前另外生成', async () => {
    const exe = writeTestFile(temp.dir, 'hello.exe', 'x');
    writeTestFile(temp.dir, 'hello.img', 'x');
    const firmware = writeTestFile(temp.dir, 'bios.nes', 'x');
    connector.next = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 }, 'input.key': {}, 'runtime.resume': {} });
    const build = new FakeBuildService('firmware-disk', firmware, exe);
    await createRunner(build).run({ build: { cwd: temp.dir }, debug: false });
    assert.equal(build.diskBuilds, 1);
  });

  it('整机 ROM 工程没有指定机型时在构建之前报错，并提示写 targetOptions.machine', async () => {
    const build = new FakeBuildService('firmware', undefined, 'x');
    await assert.rejects(createRunner(build).run({ build: { cwd: temp.dir }, debug: false }), (error: FcdevError) => error.code === 'MACHINE_MISSING' && error.hint.includes('machine'));
    assert.equal(build.builds, 0);
  });

  it('整机 ROM 工程：把产物当固件、挂上磁盘镜像开机，不输入程序名，继续运行', async () => {
    const rom = writeTestFile(temp.dir, 'bbg.bin', 'x');
    const disk = writeTestFile(temp.dir, 'dos.img', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.resume': {} });
    connector.next = client;
    const build = new FakeBuildService('firmware', undefined, rom);
    build.targetOptions = { machine: 'bbg1' };
    build.diskImage = disk;
    const result = await createRunner(build).run({ build: { cwd: temp.dir }, debug: false });
    assert.deepEqual(
      client.calls.slice(0, 3).map((call) => [call.method, call.params]),
      [
        ['machines.setRom', { machine: 'bbg1', path: rom }],
        ['machines.boot', { machine: 'bbg1', powerOn: true, resume: false }],
        ['media.floppyInsert', { drive: 0, path: disk }]
      ]
    );
    assert.ok(client.calls.some((call) => call.method === 'runtime.resume'));
    assert.equal(client.calls.filter((call) => call.method === 'input.key').length, 0);
    assert.match(result.description, /bbg\.bin（机型 bbg1，磁盘镜像 .*dos\.img）/);
    assert.equal(result.firmwareDisk, false);
    assert.equal(build.diskBuilds, 0);
  });

  it('整机 ROM 工程没有配置磁盘镜像时不放磁盘，调试运行停在开机状态', async () => {
    const rom = writeTestFile(temp.dir, 'bbg.bin', 'x');
    const client = createFakeClient({ 'machines.setRom': {}, 'machines.boot': {}, 'media.floppyInsert': {}, 'runtime.resume': {} });
    connector.next = client;
    const build = new FakeBuildService('firmware', undefined, rom);
    build.targetOptions = { machine: 'bbg1' };
    const result = await createRunner(build).run({ build: { cwd: temp.dir }, debug: true });
    assert.deepEqual(client.calls.map((call) => call.method).filter((method) => method !== 'close'), ['machines.setRom', 'machines.boot']);
    assert.deepEqual([result.debug, result.port], [true, 5701]);
  });
});
