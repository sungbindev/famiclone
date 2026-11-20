// ------------------------------------------------------------------------
// 名称：ember-fc.mjs
// 说明：用 Ember 的调试服务驱动 FC 模式下的原版 Family BASIC：启动 Ember、接入 Family BASIC 键盘、载入 ROM、按字符键入、截图、记录 CDL。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：只用来分析原版（找出哪些字节是代码）和阶段 5 的对照，不属于移植后的程序。
//       Ember 用独立的用户数据目录与调试端口启动，结束时只按自己记录的进程号关闭，不影响用户自己开着的 Ember。
// ------------------------------------------------------------------------
import { spawn } from 'node:child_process';
import { randomBytes } from 'node:crypto';
import { mkdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { locateEmulator } from '../../../../../packages/fcdev/src/emulator/index.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const REPO = resolve(HERE, '..', '..', '..', '..', '..');
const SDK = pathToFileURL(join(REPO, 'packages', 'fcdev', 'dist', 'debug', 'node.js')).href;
export const ROM = join(REPO, 'reference', 'nes-games', 'Family BASIC (Japan) (v3.0).nes');

/** 每个字符对应的物理键位与是否按住 Shift（Family BASIC 键盘按 JIS 键位）。 */
const SHIFT = 'ShiftLeft';
const PLAIN = {
  ' ': 'Space', '\n': 'Enter', '-': 'Minus', '^': 'Equal', ';': 'Semicolon', ':': 'Quote', '@': 'BracketLeft', '[': 'BracketRight',
  ']': 'Backslash', ',': 'Comma', '.': 'Period', '/': 'Slash',
};
const SHIFTED = {
  '!': 'Digit1', '"': 'Digit2', '#': 'Digit3', '$': 'Digit4', '%': 'Digit5', '&': 'Digit6', "'": 'Digit7', '(': 'Digit8', ')': 'Digit9',
  '=': 'Minus', '~': 'Equal', '+': 'Semicolon', '*': 'Quote', '<': 'Comma', '>': 'Period', '?': 'Slash',
};

/**
 * 字符转键位。
 * @param {string} char 单个字符
 * @returns {{code: string, shift: boolean}} 物理键位与是否按住 Shift
 * @throws {Error} 不支持的字符
 */
export function keyFor(char) {
  if (/^[A-Z]$/.test(char)) return { code: `Key${char}`, shift: false };
  if (/^[a-z]$/.test(char)) return { code: `Key${char.toUpperCase()}`, shift: false };
  if (/^[0-9]$/.test(char)) return { code: `Digit${char}`, shift: false };
  if (char in PLAIN) return { code: PLAIN[char], shift: false };
  if (char in SHIFTED) return { code: SHIFTED[char], shift: true };
  throw new Error(`不支持的字符：${JSON.stringify(char)}`);
}

/**
 * 启动 Ember 并连上调试服务。
 * @param {{port?: number, executable?: string}} options 调试端口与 Ember 可执行文件（不给时按 fcdev 的规则定位）
 * @returns {Promise<{client: object, pid: number, close: () => void}>} 客户端、进程号与关闭函数
 * @throws {Error} 启动失败或连不上
 */
export async function startEmber({ port = 5721, executable = locateEmulator().executable } = {}) {
  const userDataDirectory = join(tmpdir(), 'fbasic-ember', String(port));
  mkdirSync(userDataDirectory, { recursive: true });
  rmSync(join(userDataDirectory, 'run', 'debug-endpoint.json'), { force: true });
  const token = randomBytes(16).toString('hex');
  const child = spawn(executable, [`--user-data-dir=${userDataDirectory}`, `--debug-port=${port}`, `--debug-token=${token}`], { stdio: 'ignore', detached: true });
  const { connectToEmber } = await import(SDK);
  let exited = false;
  child.on('exit', () => { exited = true; });
  const deadline = Date.now() + 30000;
  let client = null;
  let lastError = null;
  while (client === null && Date.now() < deadline && !exited) {
    try {
      client = await connectToEmber({ userDataDirectory, clientName: 'fbasic-tools', connectTimeoutMs: 1000 });
    } catch (error) {
      lastError = error;
      await new Promise((done) => setTimeout(done, 500));
    }
  }
  if (client === null) {
    if (!exited) process.kill(child.pid);
    throw new Error(`连不上 Ember 的调试服务：${lastError?.message ?? '进程提前退出'}`);
  }
  return {
    client,
    pid: child.pid,
    close() {
      client.close();
      try { process.kill(child.pid); } catch { /* 已经退出 */ }
    },
  };
}

/**
 * 在 FC 模式载入 Family BASIC 并接入 Family BASIC 键盘。
 * @param {object} client 调试客户端
 * @param {number} bootFrames 载入后先运行的帧数
 */
export async function bootFamilyBasic(client, bootFrames = 120) {
  await client.api.input.setFamilyKeyboard({ connected: true });
  await client.api.cartridges.load({ path: ROM, powerOn: true, resume: false });
  await client.api.runtime.run({ frames: bootFrames, wait: true });
}

/** 运行指定帧数并等待结束。 */
export const frames = (client, count) => client.api.runtime.run({ frames: count, wait: true });

/**
 * 按字符键入文字，每个键按下与松开各保持若干帧；\n 是回车。
 * @param {object} client 调试客户端
 * @param {string} text 要键入的文字
 * @param {{hold?: number, gap?: number, enterWait?: number}} timing 按住帧数、松开帧数、回车后多等的帧数
 */
export async function typeText(client, text, { hold = 4, gap = 4, enterWait = 20 } = {}) {
  for (const char of text) {
    const { code, shift } = keyFor(char);
    if (shift) { await client.api.input.key({ code: SHIFT, down: true }); await frames(client, 2); }
    await client.api.input.key({ code, down: true });
    await frames(client, hold);
    await client.api.input.key({ code, down: false });
    await frames(client, gap);
    if (shift) { await client.api.input.key({ code: SHIFT, down: false }); await frames(client, 2); }
    if (char === '\n') await frames(client, enterWait);
  }
}

/** 截图保存成 PNG。 */
export async function screenshot(client, file) {
  const shot = await client.api.output.screenshot();
  writeFileSync(file, shot.data);
}
