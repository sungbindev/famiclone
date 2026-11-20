// ------------------------------------------------------------------------
// 名称：sb2000-run.mjs
// 说明：在 Ember 的学习机里运行 FBASIC.EXE 的会话：把 EXE 放进软盘镜像，启动模拟器，经 fcdev 公共版的 enterProgram 进入 SB-DOS 并运行程序；提供运行帧数、读写内存、按键与截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：Ember 按 fcdev 的规则定位（环境变量 FCDEV_EMULATOR，没有时用随包的 Ember）。结束时只结束自己启动的进程。
// ------------------------------------------------------------------------
import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import net from 'node:net';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { debugUserDataDirectory, enterProgram, launchEmulator, locateEmulator, runFrames, tapKey } from '../../../../../packages/fcdev/src/emulator/index.mjs';
import { getDiskPackager } from '../../../../../packages/fcdev/src/packagers/index.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const REPOSITORY = resolve(ROOT, '..', '..', '..');
const FIRMWARE = join(REPOSITORY, 'ember', 'bios', 'sb2000_bios.bin');

/** 找一个空闲端口 */
function findFreePort() {
  return new Promise((done) => {
    const server = net.createServer().listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(() => done(port));
    });
  });
}

/** 一次运行：封装 Ember 的调试客户端 */
export class Session {
  constructor({ client, pid, port }) {
    this.client = client;
    this.pid = pid;
    this.port = port;
  }

  /** 运行若干帧 */
  async frames(count) {
    await runFrames(this.client, count);
  }

  /** 按下并松开一个键（KeyboardEvent.code） */
  async tap(code) {
    await tapKey(this.client, code);
  }

  /** 按下（down为 true）或松开（false）一个键，用来按住修饰键 */
  async key(code, down) {
    await this.client.request('input.key', { code, down });
  }

  /** 读 CPU 地址空间的一段 */
  async read(address, length) {
    const result = await this.client.request('inspector.readMemory', { space: 'cpu', address, length });
    return Buffer.from(result.data);
  }

  /** 写 CPU 地址空间的一段 */
  async write(address, bytes) {
    await this.client.request('inspector.writeMemory', { space: 'cpu', address, bytes: [...bytes] });
  }

  /** CPU 与 PPU 的状态 */
  async state() {
    return this.client.request('inspector.getState', {});
  }

  /** 截图存成 PNG */
  async shot(file) {
    const result = await this.client.request('output.screenshot', {});
    writeFileSync(file, Buffer.from(result.data));
  }

  /** 关闭 Ember：只结束自己启动的进程 */
  async close() {
    this.client.close();
    try {
      execFileSync('taskkill', ['/PID', String(this.pid), '/T', '/F'], { stdio: 'ignore' });
    } catch {
      // 进程已经退出
    }
    try {
      rmSync(debugUserDataDirectory(this.port), { recursive: true, force: true, maxRetries: 10, retryDelay: 300 });
    } catch {
      // 目录还被占用时留在系统临时目录里
    }
  }
}

/**
 * 启动 Ember，放入只含 FBASIC.EXE 的软盘，进入 SB-DOS 并运行程序。
 * @param {{exe?: string, name?: string}} [options] exe 是 EXE 路径，name 是 DOS 里输入的程序名
 * @returns {Promise<Session>} 会话；返回时程序刚被启动
 */
export async function startProgram({ exe = join(ROOT, 'exe', 'build', 'sb2000', 'FBASIC.EXE'), name = 'fbasic' } = {}) {
  const emulator = locateEmulator();
  const work = join(ROOT, 'build', 'run');
  mkdirSync(work, { recursive: true });
  const imageFile = join(work, 'fbasic.img');
  getDiskPackager('fat-floppy')({ files: [{ name: 'FBASIC.EXE', data: readFileSync(exe) }], outputFile: imageFile, options: { label: 'FCDEV' } });
  const port = await findFreePort();
  const { pid, client } = await launchEmulator({ executable: emulator.executable, sb2000: { firmware: FIRMWARE, diskImage: imageFile }, debug: true, port });
  const session = new Session({ client, pid, port });
  try {
    await enterProgram(client, name);
  } catch (error) {
    await session.close();
    throw error;
  }
  return session;
}
