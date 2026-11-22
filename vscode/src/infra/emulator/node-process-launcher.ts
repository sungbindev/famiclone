// ------------------------------------------------------------------------
// 名称：node-process-launcher.ts
// 说明：用 Node 启动与结束 Ember 进程：检查端口、准备调试用的用户数据目录、启动脱离的进程、结束进
//       程。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：Ember 进程与扩展脱离（detached、不接标准输入输出），扩展退出后窗口保持打开；调试模式下每个
//       端口一个用户数据目录，放在系统临时目录的 fcdev-ember 下。
// ------------------------------------------------------------------------

import { spawn } from 'node:child_process';
import { randomBytes } from 'node:crypto';
import { mkdirSync, rmSync } from 'node:fs';
import * as net from 'node:net';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import type { EmulatorProcess, EmulatorProcessLauncher } from '../../contracts/emulator/process-launcher';
import { FcdevError } from '../../shared/errors/fcdev-error';

/** 给启动失败留出报错的机会的时间，单位毫秒。 */
const START_CONFIRM_MS = 300;

/** 调试令牌的字节数。 */
const TOKEN_BYTES = 16;

/** 启动失败的错误。 */
function startFailed(executable: string, cause: unknown): FcdevError {
  return new FcdevError(`无法启动 Ember ${executable}`, { code: 'EMULATOR_START_FAILED', cause });
}

/** 用 Node 启动与结束 Ember 进程。 */
export class NodeEmulatorProcessLauncher implements EmulatorProcessLauncher {
  /** @param temporaryDirectory 放调试用户数据目录的上级目录，默认是系统临时目录。 */
  constructor(private readonly temporaryDirectory: string = tmpdir()) {}

  /** @inheritdoc */
  assertPortFree(port: number): Promise<void> {
    return new Promise((resolve, reject) => {
      const server = net.createServer();
      server.once('error', (error) => {
        reject(new FcdevError(`端口 ${port} 已被占用`, { code: 'EMULATOR_PORT_BUSY', hint: '在设置 fcdev.emulator.debugPort 里换一个端口，或关闭占用它的 Ember', cause: error }));
      });
      server.listen(port, '127.0.0.1', () => server.close(() => resolve()));
    });
  }

  /** @inheritdoc */
  userDataDirectory(port: number): string {
    return path.join(this.temporaryDirectory, 'fcdev-ember', String(port));
  }

  /** @inheritdoc */
  prepareUserDataDirectory(port: number): string {
    const directory = this.userDataDirectory(port);
    mkdirSync(directory, { recursive: true });
    // 上一次会话遗留的端点文件，不能让客户端读到
    rmSync(path.join(directory, 'run', 'debug-endpoint.json'), { force: true });
    return directory;
  }

  /** @inheritdoc */
  start(executable: string, args: readonly string[], failOnExit: boolean): EmulatorProcess {
    const child = spawn(executable, [...args], { cwd: path.dirname(executable), stdio: 'ignore', detached: true });
    const failure = new Promise<never>((_resolve, reject) => {
      child.once('error', (error) => reject(startFailed(executable, error)));
      if (failOnExit) {
        child.once('exit', (code) => {
          reject(new FcdevError(`Ember 启动后退出了（退出码 ${code ?? '未知'}）`, { code: 'EMULATOR_START_FAILED', hint: '检查可执行文件是否是 Ember，以及端口是否被占用' }));
        });
      }
    });
    // 启动成功之后 Ember 退出也会让 failure 失败，没有人等它时不能算未处理的拒绝
    failure.catch(() => undefined);
    child.unref();
    return { pid: child.pid, failure, kill: () => child.kill() };
  }

  /** @inheritdoc */
  async confirmStarted(child: EmulatorProcess): Promise<void> {
    await Promise.race([child.failure, new Promise<void>((resolve) => setTimeout(resolve, START_CONFIRM_MS))]);
  }

  /** @inheritdoc */
  terminate(pid: number): boolean {
    try {
      process.kill(pid);
      return true;
    } catch {
      // 进程已经退出
      return false;
    }
  }

  /** @inheritdoc */
  createToken(): string {
    return randomBytes(TOKEN_BYTES).toString('hex');
  }
}
