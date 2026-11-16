// ------------------------------------------------------------------------
// 名称：ember-sb2000.mjs
// 说明：用 fcdev 在 Ember 的学习机里启动 FBASIC.EXE（调试模式，不自动运行），连上调试服务，并提供 DOS 里键入、读内存、读调试符号的辅助函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：按调试端口用独立的用户数据目录，结束时只按 fcdev 打印的进程号关闭自己启动的 Ember。
// ------------------------------------------------------------------------
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = dirname(fileURLToPath(import.meta.url));
const REPO = resolve(HERE, '..', '..', '..', '..', '..');
const FCDEV = join(REPO, 'packages', 'fcdev', 'bin', 'fcdev.mjs');
const SDK = pathToFileURL(join(REPO, 'packages', 'ember-debug-sdk', 'dist', 'node.js')).href;
const DEFAULT_EMBER = join(REPO, 'ember', 'out', 'Ember-win32-x64', 'ember.exe');

/** DOS 里键入用的物理键位：字母、数字与回车、空格。 */
export const keyFor = (char) => {
  if (/^[a-z]$/i.test(char)) return `Key${char.toUpperCase()}`;
  if (/^[0-9]$/.test(char)) return `Digit${char}`;
  const table = { ' ': 'Space', '\n': 'Enter', '.': 'Period', ':': 'Semicolon' };
  if (char in table) return table[char];
  throw new Error(`不支持的字符：${JSON.stringify(char)}`);
};

/**
 * 构建并启动 Ember（学习机），装入项目的程序，停在 SB-DOS 开机状态。
 * @param {{projectDir: string, port?: number}} options 含 fcdev.json 的目录与调试端口
 * @returns {Promise<{client: object, pid: number, symbols: Map<string, number>, close: () => void}>} 会话
 */
export async function startSb2000({ projectDir, port = 5731 }) {
  const env = { ...process.env, FCDEV_EMULATOR: process.env.FCDEV_EMULATOR ?? DEFAULT_EMBER };
  const output = execFileSync('node', [FCDEV, 'run', join(projectDir, 'src'), '--debug', '--port', String(port)], { cwd: projectDir, env, encoding: 'utf8' });
  const pid = Number(/进程 (\d+)/.exec(output)?.[1]);
  if (!Number.isInteger(pid)) throw new Error(`没有从 fcdev 的输出里取到 Ember 的进程号：${output}`);
  const { connectToEmber } = await import(SDK);
  const client = await connectToEmber({ userDataDirectory: join(tmpdir(), 'fcdev-ember', String(port)), clientName: 'fbasic-tools', connectTimeoutMs: 5000 });
  const dbg = readFileSync(join(projectDir, 'build', 'obj', 'sb2000', 'fbasic', 'fbasic.dbg'), 'latin1');
  const symbols = new Map();
  for (const match of dbg.matchAll(/^sym\s+id=\d+,name="([^"]+)",addrsize=\w+,(?:size=\d+,)?scope=0,[^\n]*?val=0x([0-9A-Fa-f]+)/gm)) symbols.set(match[1], Number.parseInt(match[2], 16));
  return {
    client,
    pid,
    symbols,
    close() {
      client.close();
      try { process.kill(pid); } catch { /* 已经退出 */ }
    },
  };
}

/** 运行指定帧数并等待结束。 */
export const frames = (client, count) => client.api.runtime.run({ frames: count, wait: true });

/** 按一下键：按下保持 hold 帧，松开后再等 gap 帧。 */
export async function tap(client, code, hold = 8, gap = 12) {
  await client.api.input.key({ code, down: true });
  await frames(client, hold);
  await client.api.input.key({ code, down: false });
  await frames(client, gap);
}

/** 在 DOS 里键入一串字符（\n 是回车）。 */
export async function typeDos(client, text) {
  for (const char of text) await tap(client, keyFor(char));
}

/**
 * 读 CPU 地址空间里的字节。
 * @param {object} client 调试客户端
 * @param {number} address 地址
 * @param {number} length 字节数
 * @returns {Promise<Uint8Array>} 内容
 */
export async function peek(client, address, length = 1) {
  const result = await client.api.inspector.readMemory({ space: 'cpu', address, length });
  return result.data;
}
