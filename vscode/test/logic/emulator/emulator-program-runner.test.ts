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

/** 假的构建服务：计划里只有运行方案与固件，记录是否构建。 */
class FakeBuildService {
  builds = 0;

  /**
   * @param mode 运行方案；null 表示目标没有运行方案。 @param firmware 项目配置里的固件。 @param
   * output 产物路径。
   */
  constructor(
    private readonly mode: 'rom' | 'firmware-disk' | null,
    private readonly firmware: string | undefined,
    private readonly output: string
  ) {}

  plan(): ReturnType<BuildService['plan']> {
    return { target: { name: 'fake', run: this.mode === null ? null : { emulator: 'ember', mode: this.mode } }, config: { firmware: this.firmware } } as never;
  }

  async build(): Promise<BuildOutcome> {
    this.builds += 1;
    return { plan: this.plan(), result: { programName: 'hello', outputFile: this.output, size: 1, compiled: 1, reused: 0 } };
  }

  buildDisk(): ReturnType<BuildService['buildDisk']> {
    return { imageFile: this.output.replace(/\.exe$/, '.img'), size: 1 };
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
});
