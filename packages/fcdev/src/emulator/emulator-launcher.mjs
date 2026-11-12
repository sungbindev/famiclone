// ------------------------------------------------------------------------
// 名称：emulator-launcher.mjs
// 说明：启动模拟器：自由运行模式直接打开窗口，调试模式设置调试端口并等待服务就绪。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：模拟器进程与命令行脱离，命令行退出后窗口保持打开；调试模式下模拟器只在收到 run 命令时才运行。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import { FcdevError } from '../shared/index.mjs';
import { DEFAULT_DEBUG_PORT, DebugClient } from './debug-client.mjs';

/** 等待调试服务就绪的上限，单位毫秒 */
const READY_TIMEOUT_MS = 30_000;

/** 轮询调试服务的间隔，单位毫秒 */
const READY_POLL_MS = 300;

/**
 * 确认本机端口空闲。端口被占用时模拟器的调试服务会静默启动失败，客户端反而可能连到别的模拟器。
 * @param {number} port 端口
 * @returns {Promise<void>}
 * @throws {FcdevError} 端口已被占用
 */
export function assertPortFree(port) {
  return new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once('error', (error) => reject(new FcdevError(`端口 ${port} 已被占用`, {
      code: 'EMULATOR_PORT_BUSY',
      hint: '用 --port 换一个端口，或关闭占用它的模拟器',
      cause: error,
    })));
    server.listen(port, '127.0.0.1', () => server.close(() => resolve()));
  });
}

/**
 * 启动模拟器。
 * @param {{executable: string, romFile: string, debug?: boolean, port?: number, readyTimeoutMs?: number}} input 模拟器程序、要加载的文件、是否调试模式与端口
 * @returns {Promise<{pid: number|undefined, client: DebugClient|null}>} 进程号；调试模式下还有已连接的客户端，调用者负责关闭
 * @throws {FcdevError} 文件不存在、无法启动，或调试服务没有在时限内就绪
 */
export async function launchEmulator({ executable, romFile, debug = false, port = DEFAULT_DEBUG_PORT, readyTimeoutMs = READY_TIMEOUT_MS }) {
  if (!existsSync(romFile)) throw new FcdevError(`找不到要加载的文件：${romFile}`, { code: 'EMULATOR_FILE_MISSING' });
  if (debug) await assertPortFree(port);
  const env = { ...process.env };
  if (debug) env.FCDEV_DEBUG_PORT = String(port);
  else delete env.FCDEV_DEBUG_PORT;

  const child = spawn(executable, [romFile], { cwd: path.dirname(executable), env, stdio: 'ignore', detached: true });
  const failure = new Promise((_, reject) => {
    child.once('error', (error) => reject(new FcdevError(`无法启动模拟器 ${executable}`, { code: 'EMULATOR_START_FAILED', cause: error })));
  });
  child.unref();
  if (!debug) {
    // 给启动失败留出报错的机会，再返回
    await Promise.race([failure, delay(300)]);
    return { pid: child.pid, client: null };
  }

  const deadline = Date.now() + readyTimeoutMs;
  while (Date.now() < deadline) {
    try {
      const client = await Promise.race([DebugClient.connect({ port, timeoutMs: 1000 }), failure]);
      const status = await client.send('status');
      if (status.head.includes('running=1')) return { pid: child.pid, client };
      client.close();
    } catch (error) {
      if (error.code === 'EMULATOR_START_FAILED') throw error;
      // 服务尚未就绪，稍后重试
    }
    await delay(READY_POLL_MS);
  }
  child.kill();
  throw new FcdevError(`等待模拟器的调试服务（端口 ${port}）就绪超时`, { code: 'EMULATOR_NOT_READY', hint: '检查端口是否被占用，或先关闭已打开的模拟器' });
}
