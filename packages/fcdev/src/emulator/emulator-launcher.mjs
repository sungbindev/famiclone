// ------------------------------------------------------------------------
// 名称：emulator-launcher.mjs
// 说明：启动 Ember：自由运行模式直接打开窗口加载程序；调试模式用独立的用户数据目录与调试端口启动，连上调试服务后载入程序并停在开机状态。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：Ember 进程与命令行脱离，命令行退出后窗口保持打开；调试模式不带 --cartridge，等连上调试服务后用 cartridges.load 载入并只上电不运行，由 debug 命令继续。
//       学习机（sb2000 参数）没有命令行入口，两种模式都经调试服务：添加固件、放入磁盘镜像、整机开机；自由模式再在 SB-DOS 里输入程序名并继续运行后断开，
//       调试模式停在开机状态（磁盘里的程序还没有被运行，由 debug 命令操作）。
//       调试服务的 SDK（fcdev 包内的 dist/debug）只在需要连接时才加载，没有构建时只影响调试模式与学习机。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { randomBytes } from 'node:crypto';
import { existsSync, mkdirSync, rmSync } from 'node:fs';
import net from 'node:net';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import { FcdevError } from '../shared/index.mjs';
import { bootSb2000, enterProgram } from './sb2000-session.mjs';

/** fcdev 的调试服务默认端口 */
export const DEFAULT_DEBUG_PORT = 5699;

/** 等待调试服务就绪的上限，单位毫秒 */
const READY_TIMEOUT_MS = 30_000;

/** 轮询调试服务的间隔，单位毫秒 */
const READY_POLL_MS = 300;

/** 调试服务连接名，显示在 Ember 的连接列表里 */
const CLIENT_NAME = 'fcdev';

/**
 * 调试模式下 Ember 使用的用户数据目录：每个端口一个，与用户平时使用的 Ember 互不影响（单实例锁与设置都按用户数据目录区分）。
 * @param {number} port 调试端口
 * @param {string} [base] 上级目录，默认是系统临时目录
 * @returns {string} 目录
 */
export function debugUserDataDirectory(port, base = tmpdir()) {
  return path.join(base, 'fcdev-ember', String(port));
}

/**
 * 确认本机端口空闲。端口被占用时 Ember 的调试服务不会启动，客户端反而可能连到别的 Ember。
 * @param {number} port 端口
 * @returns {Promise<void>}
 * @throws {FcdevError} 端口已被占用
 */
export function assertPortFree(port) {
  return new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once('error', (error) => reject(new FcdevError(`端口 ${port} 已被占用`, {
      code: 'EMULATOR_PORT_BUSY',
      hint: '用 --port 换一个端口，或关闭占用它的 Ember',
      cause: error,
    })));
    server.listen(port, '127.0.0.1', () => server.close(() => resolve()));
  });
}

/**
 * 加载调试 SDK 的 Node 入口。
 * @returns {Promise<typeof import('../../dist/debug/types/debug-sdk/src/node.js')>} SDK 的 Node 入口
 * @throws {FcdevError} SDK 没有构建
 */
async function loadSdk() {
  try {
    return await import('../../dist/debug/node.js');
  } catch (error) {
    if (error?.code === 'ERR_MODULE_NOT_FOUND') {
      throw new FcdevError('找不到调试 SDK 的构建产物', {
        code: 'DEBUG_SDK_MISSING',
        hint: '在本仓库的 packages/fcdev 目录里先运行 npm run debug-sdk:build；用 npm 安装的 fcdev 会自带',
        cause: error,
      });
    }
    throw error;
  }
}

/**
 * 启动 Ember。
 * @param {object} input 启动参数
 * @param {string} input.executable Ember 程序
 * @param {string} [input.romFile] 要加载的 .nes 文件（经典 FC）；与 sb2000 二选一
 * @param {{firmware: string, diskImage: string, programName?: string}} [input.sb2000] 学习机：固件（BIOS）、磁盘镜像，以及自由模式里要在 SB-DOS 运行的程序名
 * @param {boolean} [input.debug] 是否调试模式
 * @param {number} [input.port] 调试端口
 * @param {number} [input.readyTimeoutMs] 等待调试服务就绪的上限
 * @returns {Promise<{pid: number|undefined, client: import('../../dist/debug/types/debug-sdk/src/index.js').EmberDebugClient|null}>} 进程号；调试模式下还有已连接的客户端，调用者负责关闭
 * @throws {FcdevError} 文件不存在、无法启动、Ember 提前退出、拒绝载入，或调试服务没有在时限内就绪
 */
export async function launchEmulator({ executable, romFile, sb2000, debug = false, port = DEFAULT_DEBUG_PORT, readyTimeoutMs = READY_TIMEOUT_MS }) {
  if (romFile !== undefined && !existsSync(romFile)) throw new FcdevError(`找不到要加载的文件：${romFile}`, { code: 'EMULATOR_FILE_MISSING' });
  const cwd = path.dirname(executable);

  if (!debug && sb2000 === undefined) {
    const child = spawn(executable, [`--cartridge=${romFile}`], { cwd, stdio: 'ignore', detached: true });
    const failure = new Promise((_, reject) => {
      child.once('error', (error) => reject(new FcdevError(`无法启动 Ember ${executable}`, { code: 'EMULATOR_START_FAILED', cause: error })));
    });
    child.unref();
    // 给启动失败留出报错的机会，再返回
    await Promise.race([failure, delay(300)]);
    return { pid: child.pid, client: null };
  }

  const { connectToEmber } = await loadSdk();
  await assertPortFree(port);
  const userDataDirectory = debugUserDataDirectory(port);
  mkdirSync(userDataDirectory, { recursive: true });
  // 上一次会话遗留的端点文件，不能让客户端读到
  rmSync(path.join(userDataDirectory, 'run', 'debug-endpoint.json'), { force: true });
  const token = randomBytes(16).toString('hex');
  const child = spawn(executable, [`--user-data-dir=${userDataDirectory}`, `--debug-port=${port}`, `--debug-token=${token}`], { cwd, stdio: 'ignore', detached: true });
  const failure = new Promise((_, reject) => {
    child.once('error', (error) => reject(new FcdevError(`无法启动 Ember ${executable}`, { code: 'EMULATOR_START_FAILED', cause: error })));
    child.once('exit', (code) => reject(new FcdevError(`Ember 启动后退出了（退出码 ${code}）`, { code: 'EMULATOR_START_FAILED', hint: '检查可执行文件是否是 Ember，以及端口是否被占用' })));
  });
  // 启动成功之后 Ember 退出也会让 failure 失败，没有人等它时不能算未处理的拒绝
  failure.catch(() => {});
  child.unref();

  const deadline = Date.now() + readyTimeoutMs;
  while (Date.now() < deadline) {
    try {
      const client = await Promise.race([connectToEmber({ userDataDirectory, clientName: CLIENT_NAME, connectTimeoutMs: 1000 }), failure]);
      try {
        if (sb2000 === undefined) {
          await client.request('cartridges.load', { path: romFile, powerOn: true, resume: false });
        } else {
          await bootSb2000(client, sb2000);
          if (!debug && sb2000.programName !== undefined) {
            await enterProgram(client, sb2000.programName);
            await client.request('runtime.resume', {});
          }
        }
      } catch (error) {
        client.close();
        child.kill();
        if (error instanceof FcdevError) throw error;
        throw new FcdevError(`Ember 无法载入 ${romFile ?? sb2000.diskImage}：${error.message}`, { code: 'EMULATOR_LOAD_FAILED', cause: error });
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
