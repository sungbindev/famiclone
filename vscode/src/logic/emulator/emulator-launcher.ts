// ------------------------------------------------------------------------
// 名称：emulator-launcher.ts
// 说明：启动 Ember：自由运行模式直接打开窗口加载程序；调试模式用独立的用户数据目录与调试端口启动，
//       连上调试服务后载入程序并停在开机状态。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：Ember 进程与扩展脱离，扩展退出后窗口保持打开；调试模式不带 --cartridge，等连上调试服务后用
//       cartridges.load 载入并只上电不运行，由调试命令与 emu 工具继续。整机（firmwareDisk
//       参数）没有命令行入口，两种模式都经调试服务：添加固件、放入磁盘镜像（可选）、整机开机；
//       自由模式有程序名时再在 DOS 里输入程序名，然后继续运行并断开，调试模式停在开机
//       状态（磁盘里的程序还没有被运行）。
// ------------------------------------------------------------------------

import type { DebugSession } from '../../contracts/debug-client/debug-session';
import type { EmulatorEnvironment } from '../../contracts/emulator/emulator-environment';
import { DEFAULT_DEBUG_PORT } from '../../contracts/emulator/emulator-settings';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { bootMachine, enterProgram } from './sb2000-session';

/** 等待调试服务就绪的上限，单位毫秒。 */
const READY_TIMEOUT_MS = 30_000;

/** 轮询调试服务的间隔，单位毫秒。 */
const READY_POLL_MS = 300;

/** 调试服务连接名，显示在 Ember 的连接列表里。 */
export const CLIENT_NAME = 'fcdev';

/** 整机（固件加可选磁盘镜像）的启动内容。 */
export interface FirmwareDiskLaunch {
  /** 整机机型，如 sb2000、sb97。 */
  readonly machine: string;
  /** 固件（BIOS）文件。 */
  readonly firmware: string;
  /** 磁盘镜像；不给时软驱是空的。 */
  readonly diskImage?: string | undefined;
  /** 自由模式里要在 DOS 运行的程序名；不给时开机后直接继续运行。 */
  readonly programName?: string | undefined;
}

/** 启动 Ember 的输入。 */
export interface LaunchInput {
  /** Ember 可执行文件。 */
  readonly executable: string;
  /** 要加载的 .nes 文件（经典 FC）；与 firmwareDisk 二选一；调试模式下两者都不给就只启动 Ember。 */
  readonly romFile?: string | undefined;
  /** 整机（学习机、SB97）的启动内容。 */
  readonly firmwareDisk?: FirmwareDiskLaunch | undefined;
  /** 是否调试模式。 */
  readonly debug?: boolean | undefined;
  /** 调试端口。 */
  readonly port?: number | undefined;
  /** 等待调试服务就绪的上限。 */
  readonly readyTimeoutMs?: number | undefined;
}

/** 启动的结果。 */
export interface LaunchResult {
  /** 进程号。 */
  readonly pid: number | undefined;
  /** 调试模式下已连接的会话，调用者负责关闭；自由运行模式为 null。 */
  readonly client: DebugSession | null;
}

/** 启动要用的外部能力。 */
export type LauncherEnvironment = Pick<EmulatorEnvironment, 'files' | 'connector' | 'processes'>;

/**
 * 等一段时间。
 * @param milliseconds 毫秒数。
 */
function delay(milliseconds: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

/**
 * 启动 Ember。
 * @param env 外部能力。
 * @param input 启动参数。
 * @returns 进程号；调试模式下还有已连接的会话，调用者负责关闭。
 * @throws FcdevError 文件不存在、无法启动、Ember 提前退出、拒绝载入，或调试服务没有在时限内就绪。
 */
export async function launchEmulator(env: LauncherEnvironment, input: LaunchInput): Promise<LaunchResult> {
  const { files, connector, processes } = env;
  const { executable, romFile, firmwareDisk, debug = false, port = DEFAULT_DEBUG_PORT, readyTimeoutMs = READY_TIMEOUT_MS } = input;
  if (romFile !== undefined && files.kindOf(romFile) !== 'file') {
    throw new FcdevError(`找不到要加载的文件：${romFile}`, { code: 'EMULATOR_FILE_MISSING' });
  }

  if (!debug && firmwareDisk === undefined) {
    if (romFile === undefined) throw new FcdevError('自由运行要给出要加载的文件', { code: 'EMULATOR_PARAM_INVALID' });
    const child = processes.start(executable, [`--cartridge=${romFile}`], false);
    await processes.confirmStarted(child);
    return { pid: child.pid, client: null };
  }

  await processes.assertPortFree(port);
  const userDataDirectory = processes.prepareUserDataDirectory(port);
  const token = processes.createToken();
  const child = processes.start(executable, [`--user-data-dir=${userDataDirectory}`, `--debug-port=${port}`, `--debug-token=${token}`], true);

  const deadline = Date.now() + readyTimeoutMs;
  while (Date.now() < deadline) {
    try {
      const client = await Promise.race([child.failure, connector.connectToEmber({ userDataDirectory, clientName: CLIENT_NAME, connectTimeoutMs: 1000 })]);
      try {
        if (romFile !== undefined) {
          await client.request('cartridges.load', { path: romFile, powerOn: true, resume: false });
        } else if (firmwareDisk !== undefined) {
          await bootMachine(files, client, firmwareDisk);
          if (!debug) {
            if (firmwareDisk.programName !== undefined) await enterProgram(client, firmwareDisk.programName, firmwareDisk.machine);
            await client.request('runtime.resume', {});
          }
        }
      } catch (error) {
        client.close();
        child.kill();
        if (error instanceof FcdevError) throw error;
        throw new FcdevError(`Ember 无法载入 ${romFile ?? firmwareDisk?.diskImage ?? firmwareDisk?.firmware ?? ''}：${error instanceof Error ? error.message : String(error)}`, {
          code: 'EMULATOR_LOAD_FAILED',
          cause: error
        });
      }
      if (!debug) {
        client.close();
        return { pid: child.pid, client: null };
      }
      return { pid: child.pid, client };
    } catch (error) {
      if (error instanceof FcdevError) throw error;
      // 调试服务尚未就绪（端点文件还没写出、连接被拒绝），稍后重试
    }
    await delay(READY_POLL_MS);
  }
  child.kill();
  throw new FcdevError(`等待 Ember 的调试服务（端口 ${port}）就绪超时`, { code: 'EMULATOR_NOT_READY', hint: '检查端口是否被占用' });
}
