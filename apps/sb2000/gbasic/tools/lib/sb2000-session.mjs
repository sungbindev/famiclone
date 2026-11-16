// ------------------------------------------------------------------------
// 名称：sb2000-session.mjs
// 说明：在 Ember 里运行 GBASIC.EXE 的会话：把 EXE 放进软盘镜像，启动模拟器，开机进 SB-DOS 输入程序名；提供按键、运行帧、读内存与截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：复用 fcdev 的模拟器模块；需要环境变量 FCDEV_EMULATOR 指向 ember.exe（默认取 ember/out/Ember-win32-x64/ember.exe）。结束时只结束自己启动的进程。
// ------------------------------------------------------------------------
import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import net from 'node:net';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { debugUserDataDirectory, enterProgram, launchEmulator, locateEmulator, runFrames, tapKey } from '../../../../../packages/fcdev/src/emulator/index.mjs';
import { getDiskPackager } from '../../../../../packages/fcdev/src/packagers/index.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const GBASIC = resolve(HERE, '..', '..');
const REPOSITORY = resolve(GBASIC, '..', '..', '..');
export const FIRMWARE = join(REPOSITORY, 'reference', 'rom', 'SB2K_BIOS.NES');
const DEFAULT_EMULATOR = join(REPOSITORY, 'ember', 'out', 'Ember-win32-x64', 'ember.exe');

/**
 * 找一个空闲端口。
 * @returns {Promise<number>} 端口
 */
function findFreePort() {
  return new Promise((done) => {
    const server = net.createServer().listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(() => done(port));
    });
  });
}

/** 字符到键位：code 是 KeyboardEvent.code，shift 表示要按住 Shift（美式键盘） */
const KEYS = new Map();
for (const letter of 'ABCDEFGHIJKLMNOPQRSTUVWXYZ') KEYS.set(letter, { code: `Key${letter}`, shift: false });
for (const digit of '0123456789') KEYS.set(digit, { code: `Digit${digit}`, shift: false });
const SYMBOLS = [[' ', 'Space', false], ['\n', 'Enter', false], ['-', 'Minus', false], ['=', 'Equal', false], ['+', 'Equal', true], [',', 'Comma', false], ['.', 'Period', false], ['/', 'Slash', false], [';', 'Semicolon', false], [':', 'Semicolon', true], ['"', 'Quote', true], ['(', 'Digit9', true], [')', 'Digit0', true], ['*', 'Digit8', true], ['?', 'Slash', true], ['<', 'Comma', true], ['>', 'Period', true], ['$', 'Digit4', true], ['!', 'Digit1', true], ['%', 'Digit5', true], ['&', 'Digit7', true], ['#', 'Digit3', true], ['@', 'Digit2', true]];
for (const [char, code, shift] of SYMBOLS) KEYS.set(char, { code, shift });

/** 一次运行：封装 Ember 的调试客户端 */
export class Session {
  /**
   * @param {object} parts 组成部分
   * @param {import('ember-debug-sdk').EmberDebugClient} parts.client 调试客户端
   * @param {number} parts.pid Ember 进程号
   * @param {number} parts.port 调试端口
   * @param {string} parts.imageFile 镜像路径
   */
  constructor({ client, pid, port, imageFile }) {
    this.client = client;
    this.pid = pid;
    this.port = port;
    this.imageFile = imageFile;
  }

  /**
   * 按下并松开一个键，期间运行若干帧。
   * @param {string} code KeyboardEvent.code
   * @returns {Promise<void>}
   */
  tap(code) {
    return tapKey(this.client, code);
  }

  /**
   * 运行若干帧。
   * @param {number} frames 帧数
   * @returns {Promise<void>}
   */
  async frames(frames) {
    await runFrames(this.client, frames);
  }

  /**
   * 按下一个键（可带 Shift），保持若干帧再松开。
   * @param {string} code KeyboardEvent.code
   * @param {{shift?: boolean, down?: number, up?: number}} [options] shift 为 true 时先按住左 Shift；down、up 是按下与松开后运行的帧数
   * @returns {Promise<void>}
   */
  async press(code, { shift = false, down = 8, up = 12 } = {}) {
    if (shift) { await this.client.request('input.key', { code: 'ShiftLeft', down: true }); await runFrames(this.client, 2); }
    await this.client.request('input.key', { code, down: true });
    await runFrames(this.client, down);
    await this.client.request('input.key', { code, down: false });
    await runFrames(this.client, up);
    if (shift) { await this.client.request('input.key', { code: 'ShiftLeft', down: false }); await runFrames(this.client, 2); }
  }

  /**
   * 逐字输入文字，\n 是回车。
   * @param {string} text 文字，只含键盘上能直接按出来的 ASCII 字符
   * @returns {Promise<void>}
   */
  async type(text) {
    for (const char of text) {
      const key = KEYS.get(char);
      if (!key) throw new Error(`没有 ${JSON.stringify(char)} 的键位`);
      await this.press(key.code, { shift: key.shift });
    }
  }

  /**
   * 读 CPU 地址空间的一段内存。
   * @param {number} address 起始地址
   * @param {number} length 字节数
   * @returns {Promise<Buffer>} 内容
   */
  async read(address, length) {
    const result = await this.client.request('inspector.readMemory', { space: 'cpu', address, length });
    return Buffer.from(result.data);
  }

  /**
   * 截取当前画面并存成文件。
   * @param {string} file PNG 文件路径
   * @returns {Promise<void>}
   */
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
 * 启动 Ember，放入只含 GBASIC.EXE 的软盘，经 fcdev 的 enterProgram 进入 SB-DOS 并运行程序。
 * @param {{name?: string, exe?: string}} [options] name 是 DOS 里输入的程序名，exe 是 EXE 路径
 * @returns {Promise<Session>} 会话；返回时程序已经开始运行
 */
export async function startGbasic({ name = 'gbasic', exe = join(GBASIC, 'exe', 'build', 'sb2000', 'GBASIC.EXE') } = {}) {
  process.env.FCDEV_EMULATOR ??= DEFAULT_EMULATOR;
  const emulator = locateEmulator();
  const work = join(GBASIC, 'build', 'session');
  mkdirSync(work, { recursive: true });
  const imageFile = join(work, 'gbasic.img');
  getDiskPackager('fat-floppy')({
    files: [{ name: 'GBASIC.EXE', data: readFileSync(exe) }],
    outputFile: imageFile,
    options: { label: 'FCDEV' },
  });
  const port = await findFreePort();
  const { pid, client } = await launchEmulator({ executable: emulator.executable, sb2000: { firmware: FIRMWARE, diskImage: imageFile }, debug: true, port });
  const session = new Session({ client, pid, port, imageFile });
  try {
    await enterProgram(client, name);
  } catch (error) {
    await session.close();
    throw error;
  }
  return session;
}
