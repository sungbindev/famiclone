// ------------------------------------------------------------------------
// 名称：real-emulator.ts
// 说明：真实 Ember 测试的公共工具：找 Ember 与固件、组装用真实调试连接的模拟器操作、构建测试程序放
//       进软盘镜像并在 Ember 里运行、读出测试程序记录的检查结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：测试程序放在 test/fixtures/sb2000/，用 sb-test.h 记录检查结果；整机经调试服务启动（固件、软
//       盘镜像、开机），在 SB-DOS 里输入程序名运行，再对停靠标志 sb_test_stop 设写断点，运行到
//       sb_test_sync、sb_test_done。需要 Ember（环境变量 FCDEV_TEST_EMULATOR）与 cc65；固件取环境变
//       量FCDEV_TEST_FIRMWARE，没有时用仓库里的 ember/roms/sb2000_bios.bin；缺一由调用者跳过，设置
//       FCDEV_SKIP_GUI_TESTS=1 也跳过。调试端口自动选空闲端口，只结束自己启动的 Ember。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import * as net from 'node:net';
import * as path from 'node:path';
import type { DebugSession } from '../../src/contracts/debug-client/debug-session';
import type { InputMouseParams } from '../../src/contracts/debug-client/generated/api';
import type { EmulatorEnvironment } from '../../src/contracts/emulator/emulator-environment';
import { FileEndpointResolver } from '../../src/infra/debug-client/endpoint-file';
import { openWebSocket } from '../../src/infra/debug-client/web-socket-transport';
import { NodeEmulatorProcessLauncher } from '../../src/infra/emulator/node-process-launcher';
import { NodeFileSystem } from '../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../src/infra/resources/extension-resources-factory';
import { EmberConnector } from '../../src/logic/debug-client/ember-connector';
import { CoreEmulatorOperations } from '../../src/logic/emulator/core-emulator-operations';
import { launchEmulator } from '../../src/logic/emulator/emulator-launcher';
import { EmulatorProgramRunner } from '../../src/logic/emulator/emulator-program-runner';
import { enterProgram, runFrames, tapKey } from '../../src/logic/emulator/sb2000-session';
import { FileTargetProfileReader } from '../../src/logic/targets/file-target-profile-reader';
import { packageDisk } from '../../src/shared/packaging/program-packager';
import { createRealBuildEnvironment, EXTENSION_DIRECTORY, HAS_TOOLCHAIN, writeTestFile, type TempDir } from './build-fixtures';
import { FakeSettings } from './emulator-fixtures';

/** 测试程序目录。 */
const FIXTURE_DIR = path.join(EXTENSION_DIRECTORY, 'test', 'fixtures', 'sb2000');

/** 仓库里的学习机 BIOS，没有设置 FCDEV_TEST_FIRMWARE 时使用。 */
const REPOSITORY_FIRMWARE = path.resolve(EXTENSION_DIRECTORY, '..', 'ember', 'roms', 'sb2000_bios.bin');

/** 物理键位（KeyboardEvent.code）：测试里用到的按键。 */
export const KEY: Readonly<Record<string, string>> = {
  enter: 'Enter',
  escape: 'Escape',
  f1: 'F1',
  0: 'Digit0',
  1: 'Digit1',
  2: 'Digit2',
  3: 'Digit3',
  4: 'Digit4',
  5: 'Digit5',
  6: 'Digit6',
  7: 'Digit7',
  8: 'Digit8',
  9: 'Digit9',
  a: 'KeyA',
  b: 'KeyB',
  c: 'KeyC'
};

/** 测试程序名，必须是 8.3 名字。 */
const PROGRAM_NAME = 'sbtest';

/** 读 CPU 地址空间的空间标识。 */
const CPU_SPACE = 'cpu';

/** 软驱编号。 */
const DRIVE = 0;

/** 一次运行最多等待的毫秒数。 */
const RUN_TIMEOUT_MS = 120_000;

/** 真实 Ember 的位置：环境变量 FCDEV_TEST_EMULATOR 指向的文件，不满足条件时为 undefined。 */
export const REAL_EMBER: string | undefined = (() => {
  const executable = process.env.FCDEV_TEST_EMULATOR;
  if (process.env.FCDEV_SKIP_GUI_TESTS === '1' || executable === undefined || executable === '') return undefined;
  return existsSync(executable) ? executable : undefined;
})();

/** 学习机固件的位置：环境变量 FCDEV_TEST_FIRMWARE，没有时用仓库里的；文件不存在时为 undefined。 */
export const REAL_FIRMWARE: string | undefined = (() => {
  const firmware = process.env.FCDEV_TEST_FIRMWARE ?? REPOSITORY_FIRMWARE;
  return existsSync(firmware) ? firmware : undefined;
})();

/** 需要 Ember 与 cc65 的测试是否可以运行。 */
export const CAN_RUN_REAL_EMBER = REAL_EMBER !== undefined && HAS_TOOLCHAIN;

/** 需要 Ember、cc65 与学习机固件的测试是否可以运行。 */
export const CAN_RUN_REAL_SB2000 = CAN_RUN_REAL_EMBER && REAL_FIRMWARE !== undefined;

/**
 * 找一个空闲端口。
 * @returns 端口。
 */
export function findFreePort(): Promise<number> {
  return new Promise((resolve) => {
    const server = net.createServer().listen(0, '127.0.0.1', () => {
      const { port } = server.address() as net.AddressInfo;
      server.close(() => resolve(port));
    });
  });
}

/** 真实的模拟器环境与基于它的操作。 */
export interface RealEmulator {
  /** 环境。 */
  readonly environment: EmulatorEnvironment;
  /** 模拟器操作。 */
  readonly operations: CoreEmulatorOperations;
  /** 构建并运行程序。 */
  readonly runner: EmulatorProgramRunner;
  /** 使用的调试端口。 */
  readonly port: number;
  /** 进程启动，用来清理调试用户数据目录。 */
  readonly processes: NodeEmulatorProcessLauncher;
}

/**
 * 组装真实的模拟器环境：真实的调试连接与进程启动，Ember 位置与端口由调用者给出。
 * @param executable Ember 可执行文件；不给时模拟“没有设置位置”。
 * @param port 调试端口。
 * @returns 环境与进程启动。
 */
export function createRealEnvironment(executable: string | undefined, port: number): { environment: EmulatorEnvironment; processes: NodeEmulatorProcessLauncher } {
  const files = new NodeFileSystem();
  const resources = createExtensionResources(EXTENSION_DIRECTORY);
  const processes = new NodeEmulatorProcessLauncher();
  const environment: EmulatorEnvironment = {
    files,
    profiles: new FileTargetProfileReader(files, resources),
    connector: new EmberConnector(openWebSocket, new FileEndpointResolver()),
    settings: new FakeSettings(executable, port),
    processes
  };
  return { environment, processes };
}

/**
 * 组装真实的模拟器环境：真实的调试连接与进程启动，Ember 位置取自环境变量，端口是空闲端口。
 * @returns 真实环境。
 */
export async function createRealEmulator(): Promise<RealEmulator> {
  assert.ok(REAL_EMBER, '没有可用的 Ember（环境变量 FCDEV_TEST_EMULATOR）');
  const port = await findFreePort();
  const { environment, processes } = createRealEnvironment(REAL_EMBER, port);
  return {
    environment,
    operations: new CoreEmulatorOperations(environment),
    runner: new EmulatorProgramRunner(createRealBuildEnvironment().service, environment),
    port,
    processes
  };
}

/**
 * 清理调试用户数据目录：Ember 退出需要一点时间，目录还被占用时放弃清理也不影响测试结果。
 * @param processes 进程启动。
 * @param port 调试端口。
 */
export function cleanUserData(processes: NodeEmulatorProcessLauncher, port: number): void {
  try {
    rmSync(processes.userDataDirectory(port), { recursive: true, force: true, maxRetries: 10, retryDelay: 300 });
  } catch {
    // 临时目录留在系统临时目录里，下次启动会覆盖
  }
}

/**
 * 从链接器的调试信息里取符号地址（映射文件只列被其他模块引用的导出）。
 * @param debugText 调试信息文件内容。
 * @param symbol 符号名，含前导下划线。
 * @returns 地址。
 */
function findSymbol(debugText: string, symbol: string): number {
  const match = new RegExp(`^sym\\b.*name="${symbol}",.*?\\bval=0x([0-9A-Fa-f]+)`, 'm').exec(debugText);
  assert.ok(match?.[1], `调试信息里没有符号 ${symbol}`);
  return Number.parseInt(match[1], 16) & 0xffff;
}

/** 测试程序记录的一次检查结果。 */
export interface CheckResult {
  ok: boolean;
  line: number;
  value: number;
}

/** 一次测试运行的组成部分。 */
interface SessionParts {
  readonly client: DebugSession;
  readonly pid: number;
  readonly port: number;
  readonly symbols: Readonly<Record<string, number>>;
  readonly imageFile: string;
  readonly processes: NodeEmulatorProcessLauncher;
  /** 测试程序里记录变量的名字前缀，默认 sb_test（步步高的测试程序用 bbg_test）。 */
  readonly prefix?: string;
  /** 停靠时是否核对 $8000 窗口里的学习机程序文件头；步步高没有这个文件头，为 false。 */
  readonly checkHeader?: boolean;
}

/** 一次测试运行：封装 Ember 与测试程序的符号。 */
export class Sb2000Session {
  /** @param parts 调试会话、Ember 进程号、调试端口、符号地址、镜像路径与进程启动。 */
  constructor(private readonly parts: SessionParts) {}

  /** 测试程序记录变量的名字前缀。 */
  private get prefix(): string {
    return this.parts.prefix ?? 'sb_test';
  }

  /**
   * 按下并松开一个键，期间运行若干帧让 BIOS 读到。
   * @param code 物理键位，取 KEY 里的值。
   */
  tap(code: string): Promise<void> {
    return tapKey(this.parts.client, code);
  }

  /**
   * 设置 0 号手柄口的按键。
   * @param buttons 按键掩码：1 A、2 B、4 选择、8 开始、16 上、32 下、64 左、128 右。
   */
  async pad(buttons: number): Promise<void> {
    await this.parts.client.request('input.pad', { port: 0, buttons });
  }

  /**
   * 发鼠标事件。
   * @param params input.mouse 的参数。
   */
  async mouse(params: InputMouseParams): Promise<void> {
    await this.parts.client.request('input.mouse', params);
  }

  /**
   * 读 CPU 地址空间的一段内存。
   * @param address 起始地址。
   * @param length 字节数。
   * @returns 内容。
   */
  async read(address: number, length: number): Promise<Buffer> {
    const result = await this.parts.client.request('inspector.readMemory', { space: CPU_SPACE, address, length });
    return Buffer.from(result.data ?? new Uint8Array());
  }

  /**
   * 读测试程序里的一个全局变量。
   * @param name 变量名，不含下划线。
   * @param length 字节数。
   * @returns 内容。
   */
  readVariable(name: string, length: number): Promise<Buffer> {
    return this.read(this.parts.symbols[name] ?? 0, length);
  }

  /**
   * 运行到测试程序的停靠点（sb_test_sync 或 sb_test_done）。
   * @param maxFrames 最多运行的帧数。
   * @returns 停在哪个停靠点。
   */
  async runToStop(maxFrames = 6000): Promise<'sync' | 'done'> {
    for (let done = 0; done < maxFrames; ) {
      const run = await runFrames(this.parts.client, 300);
      done += run.frames;
      if (run.stopped !== 'breakpoint') {
        assert.ok(run.stopped === 'completed' || run.stopped === 'interrupted', `运行意外停止：${String(run.stopped)}`);
        continue;
      }
      // DOS 外壳也在 $8000 窗口里运行，窗口里是本程序（文件头 AA 55）时才算到达
      if (this.parts.checkHeader !== false) {
        const header = await this.read(0x8000, 5);
        if (header[3] !== 0xaa || header[4] !== 0x55) continue;
      }
      const flag = (await this.readVariable(`${this.prefix}_stop`, 1))[0];
      if (flag === 0) continue; // 启动代码清零 BSS 时也写这个单元
      if (flag === 1) return 'done';
      if (flag === 2) return 'sync';
      assert.fail(`停在了意外的位置：停靠标志为 ${String(flag)}`);
    }
    return assert.fail(`运行 ${maxFrames} 帧后测试程序仍没有到达停靠点`);
  }

  /**
   * 运行到步步高测试程序的停靠点：每运行若干帧读一次停靠标志（不设断点）。
   * 程序在 bbt_sync 里写 2 并等待，结束时写 1。
   * 停在 sync 时调用者做完截图等操作后要调用 resume() 让程序继续。
   * @param maxFrames 最多运行的帧数。
   * @returns 停在哪个停靠点。
   */
  async runToStopPolled(maxFrames = 6000): Promise<'sync' | 'done'> {
    const step = 20;
    for (let done = 0; done < maxFrames; done += step) {
      await runFrames(this.parts.client, step);
      const flag = (await this.readVariable(`${this.prefix}_stop`, 1))[0];
      if (flag === 1) return 'done';
      if (flag === 2) return 'sync';
    }
    return assert.fail(`运行 ${maxFrames} 帧后测试程序仍没有到达停靠点`);
  }

  /** 让停在 sync 的步步高测试程序继续运行（把停靠标志清零）。 */
  async resume(): Promise<void> {
    await this.parts.client.request('inspector.writeMemory', { space: CPU_SPACE, address: this.parts.symbols[`${this.prefix}_stop`] ?? 0, bytes: [0] });
  }

  /**
   * 读出测试程序记录的全部检查结果。
   * @returns 检查结果。
   */
  async readChecks(): Promise<CheckResult[]> {
    const count = (await this.readVariable(`${this.prefix}_count`, 1))[0] ?? 0;
    const max = 80;
    const ok = await this.readVariable(`${this.prefix}_ok`, max);
    const lines = await this.readVariable(`${this.prefix}_line`, max * 2);
    const values = await this.readVariable(`${this.prefix}_value`, max * 2);
    return Array.from({ length: Math.min(count, max) }, (_, index) => ({
      ok: ok[index] === 1,
      line: lines.readUInt16LE(index * 2),
      value: values.readUInt16LE(index * 2)
    }));
  }

  /**
   * 断言全部检查通过；失败时列出源文件行号与实际值。
   * @param expectedCount 应该执行的检查次数。
   */
  async assertAllPassed(expectedCount: number): Promise<void> {
    const checks = await this.readChecks();
    const where = (line: number): string => (this.prefix === 'sb_test' ? `第 ${line} 行` : `检查 ${line}`);
    const failed = checks.filter((check) => !check.ok).map((check) => `${where(check.line)}（实际值 ${check.value}，$${check.value.toString(16)}）`);
    assert.deepEqual(failed, [], `失败的检查：${failed.join('；')}`);
    assert.equal(checks.length, expectedCount, `应执行 ${expectedCount} 次检查，实际 ${checks.length} 次`);
  }

  /**
   * 把软盘镜像写回文件并读出，用来核对程序对磁盘的修改。
   * @returns 镜像内容。
   */
  async saveDisk(): Promise<Buffer> {
    await this.parts.client.request('media.floppySave', { drive: DRIVE });
    return readFileSync(this.parts.imageFile);
  }

  /**
   * 截取当前画面。
   * @returns PNG 文件内容。
   */
  async shot(): Promise<Buffer> {
    const result = await this.parts.client.request('output.screenshot', {});
    return Buffer.from(result.data ?? new Uint8Array());
  }

  /** 关闭 Ember：只结束自己启动的进程（含它的子进程）。 */
  close(): void {
    this.parts.client.close();
    try {
      if (process.platform === 'win32') execFileSync('taskkill', ['/PID', String(this.parts.pid), '/T', '/F'], { stdio: 'ignore' });
      else process.kill(this.parts.pid);
    } catch {
      // 进程已经退出
    }
    cleanUserData(this.parts.processes, this.parts.port);
  }
}

/** 放进镜像的一个文件。 */
export interface ImageFile {
  name: string;
  data: Buffer;
}

/**
 * 在临时目录写入一个文本文件的镜像条目，便于给文件类测试提供数据。
 * @param name 8.3 文件名。
 * @param content 内容。
 * @returns 镜像文件项。
 */
export function imageFile(name: string, content: string | Buffer): ImageFile {
  return { name, data: Buffer.isBuffer(content) ? content : Buffer.from(content) };
}

/** 启动测试程序的选项。 */
export interface StartOptions {
  /** fixtures/sb2000 下的测试程序文件名。 */
  readonly sources: readonly string[];
  /** 一并放进镜像的文件。 */
  readonly extraFiles?: readonly ImageFile[];
  /** 另外构建、一并放进镜像的子程序（name 是 8.3 主名）。 */
  readonly children?: ReadonlyArray<{ name: string; source: string }>;
  /** 写进 fcdev.json 的 targetOptions。 */
  readonly targetOptions?: Record<string, unknown>;
}

/**
 * 构建测试程序，放进软盘镜像并在 Ember 里启动到 SB-DOS，输入程序名运行它。返回时测试程序已经开始运
 * 行，
 * 并已对停靠标志设好写断点，调用 session.runToStop() 运行到停靠点。
 * @param temp 临时目录。
 * @param options 选项。
 * @returns 会话。
 */
export async function startSb2000Program(temp: TempDir, { sources, extraFiles = [], children = [], targetOptions = {} }: StartOptions): Promise<Sb2000Session> {
  assert.ok(REAL_EMBER && REAL_FIRMWARE, '没有可用的 Ember 与固件');
  const emulator = await createRealEmulator();
  const build = createRealBuildEnvironment().service;
  writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: PROGRAM_NAME, firmware: REAL_FIRMWARE, targetOptions }));
  mkdirSync(path.join(temp.dir, 'src'), { recursive: true });
  for (const name of ['sb-test.h', 'sb-test.c', ...sources]) {
    copyFileSync(path.join(FIXTURE_DIR, name), path.join(temp.dir, 'src', name));
  }
  const { plan, result } = await build.build({ pathArg: 'src', cwd: temp.dir });

  const childFiles: ImageFile[] = [];
  for (const child of children) {
    const childDir = path.join(temp.dir, `child-${child.name}`);
    writeTestFile(childDir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: child.name }));
    mkdirSync(path.join(childDir, 'src'), { recursive: true });
    copyFileSync(path.join(FIXTURE_DIR, child.source), path.join(childDir, 'src', child.source));
    const built = await build.build({ pathArg: 'src', cwd: childDir });
    childFiles.push({ name: path.basename(built.result.outputFile), data: readFileSync(built.result.outputFile) });
  }

  const image = path.join(temp.dir, 'test.img');
  writeFileSync(image, packageDisk('fat-floppy', [{ name: path.basename(result.outputFile), data: readFileSync(result.outputFile) }, ...childFiles, ...extraFiles], { label: 'FCDEV' }));

  const debugText = readFileSync(path.join(plan.objectDir, `${PROGRAM_NAME}.dbg`), 'utf8');
  const symbols: Record<string, number> = {};
  for (const symbol of ['sb_test_stop', 'sb_test_count', 'sb_test_ok', 'sb_test_line', 'sb_test_value']) {
    symbols[symbol] = findSymbol(debugText, `_${symbol}`);
  }

  const { pid, client } = await launchEmulator(emulator.environment, {
    executable: REAL_EMBER,
    firmwareDisk: { machine: 'sb2000', firmware: REAL_FIRMWARE, diskImage: image },
    debug: true,
    port: emulator.port
  });
  assert.ok(client !== null && pid !== undefined, 'Ember 没有给出调试连接');
  const session = new Sb2000Session({ client, pid, port: emulator.port, symbols, imageFile: image, processes: emulator.processes });
  try {
    await enterProgram(client, PROGRAM_NAME);
    // 断点要在输入完程序名之后再设：设早了会打断按键所需的帧数
    await client.request('inspector.addBreakpoint', { kind: 'write', address: symbols.sb_test_stop ?? 0 }, { timeoutMs: RUN_TIMEOUT_MS });
  } catch (error) {
    session.close();
    throw error;
  }
  return session;
}

/** 步步高测试程序目录。 */
const BBG_FIXTURE_DIR = path.join(EXTENSION_DIRECTORY, 'test', 'fixtures', 'bbg');

/** 仓库里的步步高软驱一号 BIOS，没有设置 FCDEV_TEST_BBG_FIRMWARE 时使用。 */
const REPOSITORY_BBG_FIRMWARE = path.resolve(EXTENSION_DIRECTORY, '..', 'ember', 'roms', 'bbg1_bios.bin');

/** 仓库里的步步高系统盘（BBGDOS 3.0），测试程序放在它的副本上。 */
const REPOSITORY_BBG_SYSTEM_DISK = path.resolve(EXTENSION_DIRECTORY, '..', 'reference', 'floppy-images', 'bbg', 'BBGDOS.IMG');

/** 步步高测试程序名，必须是 8.3 名字。 */
const BBG_PROGRAM_NAME = 'bbgtest';

/** 步步高测试程序的公共源文件。 */
const BBG_COMMON_SOURCES = ['bbg-test.inc', 'bbg-test.s'];

/** 步步高测试程序记录变量的名字前缀。 */
const BBG_TEST_PREFIX = 'bbg_test';

/** 步步高固件位置：环境变量 FCDEV_TEST_BBG_FIRMWARE，没有时用仓库里的；不存在时为 undefined。 */
export const REAL_BBG_FIRMWARE: string | undefined = (() => {
  const firmware = process.env.FCDEV_TEST_BBG_FIRMWARE ?? REPOSITORY_BBG_FIRMWARE;
  return existsSync(firmware) ? firmware : undefined;
})();

/** 需要 Ember、cc65、步步高固件与系统盘的测试是否可以运行。 */
export const CAN_RUN_REAL_BBG = CAN_RUN_REAL_EMBER && REAL_BBG_FIRMWARE !== undefined && existsSync(REPOSITORY_BBG_SYSTEM_DISK);

/**
 * 构建步步高测试程序，放进系统盘副本，在 bbg1 里启动 BBGDOS 并运行它。
 * 返回时测试程序已经开始运行，调用 session.runToStopPolled() 运行到停靠点。
 * @param temp 临时目录。
 * @param sources fixtures/bbg 下的测试程序文件名。
 * @returns 会话，检查结果的变量前缀是 bbg_test。
 */
export async function startBbgProgram(temp: TempDir, sources: readonly string[]): Promise<Sb2000Session> {
  assert.ok(REAL_EMBER && REAL_BBG_FIRMWARE, '没有可用的 Ember 与步步高固件');
  const emulator = await createRealEmulator();
  const build = createRealBuildEnvironment().service;
  writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'bbg', name: BBG_PROGRAM_NAME, firmware: REAL_BBG_FIRMWARE, diskImage: REPOSITORY_BBG_SYSTEM_DISK }));
  mkdirSync(path.join(temp.dir, 'src'), { recursive: true });
  for (const name of [...BBG_COMMON_SOURCES, ...sources]) {
    copyFileSync(path.join(BBG_FIXTURE_DIR, name), path.join(temp.dir, 'src', name));
  }
  const { plan, disk } = await build.build({ pathArg: 'src', cwd: temp.dir });
  assert.ok(disk, '构建没有生成磁盘镜像');

  const debugText = readFileSync(path.join(plan.objectDir, `${BBG_PROGRAM_NAME}.dbg`), 'utf8');
  const symbols: Record<string, number> = {};
  for (const suffix of ['stop', 'count', 'ok', 'line', 'value']) {
    const name = `${BBG_TEST_PREFIX}_${suffix}`;
    symbols[name] = findSymbol(debugText, name);
  }

  const { pid, client } = await launchEmulator(emulator.environment, {
    executable: REAL_EMBER,
    firmwareDisk: { machine: 'bbg1', firmware: REAL_BBG_FIRMWARE, diskImage: disk.imageFile },
    debug: true,
    port: emulator.port
  });
  assert.ok(client !== null && pid !== undefined, 'Ember 没有给出调试连接');
  const session = new Sb2000Session({ client, pid, port: emulator.port, symbols, imageFile: disk.imageFile, processes: emulator.processes, prefix: BBG_TEST_PREFIX, checkHeader: false });
  try {
    await enterProgram(client, BBG_PROGRAM_NAME, 'bbg1');
  } catch (error) {
    session.close();
    throw error;
  }
  return session;
}
