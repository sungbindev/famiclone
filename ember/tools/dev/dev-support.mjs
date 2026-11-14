/*
 * ------------------------------------------------------------------------
 * 名称：dev-support.mjs
 * 说明：开发期脚本（冒烟、延迟测量）共用的工具：启动开发版并经 CDP 连接页面、等待条件、截图、发按键，以及只按 PID 结束自己启动的进程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只结束本脚本启动的 forge 进程树与它下面的核心进程（按 PID），不按进程名结束任何进程；用户数据目录是临时目录，
 *       不影响已打开的 Ember，也避开单实例锁。只在 Ember 项目目录下运行（启动前检查 package.json 的名称）。
 * ------------------------------------------------------------------------
 */

import { execFileSync, spawn, spawnSync } from 'node:child_process';
import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

/** Ember 项目根目录。 */
export const projectRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');

/** 等待开发版启动并出现页面的最长时间（毫秒）：首次要先构建主进程、预加载脚本与界面。 */
export const STARTUP_TIMEOUT_MS = 180_000;
/** 等待其他条件的最长时间（毫秒）。 */
export const WAIT_TIMEOUT_MS = 20_000;

/** 解析命令行：--名称=值 或 --名称。 */
export function parseArguments(argv) {
  const options = new Map();
  for (const argument of argv) {
    const match = /^--([^=]+)(?:=(.*))?$/.exec(argument);
    if (match) options.set(match[1], match[2] ?? 'true');
  }
  return options;
}

export function sleep(milliseconds) {
  return new Promise((done) => setTimeout(done, milliseconds));
}

/** 轮询直到条件成立或超时，返回最后一次的值。 */
export async function waitFor(produce, accept, timeout = WAIT_TIMEOUT_MS, interval = 100) {
  const deadline = Date.now() + timeout;
  let value = await produce();
  while (!accept(value) && Date.now() < deadline) {
    await sleep(interval);
    value = await produce();
  }
  return value;
}

/** 结束一个进程及其全部子进程（按 PID，Windows 用 taskkill /T）。 */
export function killTree(pid) {
  if (pid === undefined) return;
  try {
    if (process.platform === 'win32') execFileSync('taskkill', ['/PID', String(pid), '/T', '/F'], { stdio: 'ignore' });
    else process.kill(-pid, 'SIGKILL');
  } catch {
    // 进程已经退出。
  }
}

/** 列出 forge 进程树下命令行含指定文字的进程 PID（Windows）。 */
export function findDescendants(rootPid, commandLineMarker) {
  const script = `
    $all = Get-CimInstance Win32_Process
    $ids = New-Object System.Collections.Generic.HashSet[int]
    [void]$ids.Add(${rootPid})
    do { $added = $false
      foreach ($p in $all) { if ($ids.Contains([int]$p.ParentProcessId) -and $ids.Add([int]$p.ProcessId)) { $added = $true } }
    } while ($added)
    $all | Where-Object { $ids.Contains([int]$_.ProcessId) -and $_.CommandLine -like '*${commandLineMarker}*' } | ForEach-Object { $_.ProcessId }`;
  const output = spawnSync('powershell', ['-NoProfile', '-NonInteractive', '-Command', script], { encoding: 'utf8' }).stdout ?? '';
  return output
    .split(/\r?\n/)
    .map((line) => Number(line.trim()))
    .filter((value) => Number.isInteger(value) && value > 0);
}

/** 一条到页面（或浏览器）的 CDP 连接。 */
export async function connectCdp(url) {
  const socket = new WebSocket(url);
  await new Promise((done, fail) => {
    socket.addEventListener('open', done, { once: true });
    socket.addEventListener('error', () => fail(new Error('无法连接 CDP')), { once: true });
  });
  let counter = 0;
  const waiting = new Map();
  socket.addEventListener('message', (event) => {
    const message = JSON.parse(String(event.data));
    const entry = waiting.get(message.id);
    if (entry === undefined) return;
    waiting.delete(message.id);
    if (message.error) entry.fail(new Error(message.error.message));
    else entry.done(message.result);
  });
  return {
    send(method, params = {}) {
      counter += 1;
      const id = counter;
      return new Promise((done, fail) => {
        waiting.set(id, { done, fail });
        socket.send(JSON.stringify({ id, method, params }));
      });
    },
    async evaluate(expression) {
      const result = await this.send('Runtime.evaluate', { expression, awaitPromise: true, returnByValue: true });
      if (result.exceptionDetails) throw new Error(result.exceptionDetails.exception?.description ?? result.exceptionDetails.text);
      return result.result.value;
    },
    close: () => socket.close()
  };
}

/** 截图并保存为 PNG。 */
export async function saveScreenshot(page, path) {
  const { data } = await page.send('Page.captureScreenshot', { format: 'png' });
  writeFileSync(path, Buffer.from(data, 'base64'));
  console.log(`截图：${path}`);
}

/** 经 CDP 按下并松开一个键（key 是按键名，code 是物理键位，keyCode 是 Windows 虚拟键码）。 */
export async function pressKey(page, { key, code, keyCode }, holdMs) {
  const common = { key, code, windowsVirtualKeyCode: keyCode, nativeVirtualKeyCode: keyCode };
  await page.send('Input.dispatchKeyEvent', { type: 'rawKeyDown', ...common });
  await sleep(holdMs);
  await page.send('Input.dispatchKeyEvent', { type: 'keyUp', ...common });
}

/**
 * 启动开发版（electron-forge start）：临时用户数据目录，主进程直接加载一张卡带，开 CDP 调试端口。
 * @param {{ port: number, romPath: string, windowPlacement?: { x: number, y: number, width: number, height: number } }} settings CDP 端口、卡带路径，以及可选的预置窗口位置（写进临时数据目录的界面状态，尺寸低于最小尺寸时取最小尺寸）。
 * @returns 句柄：pid、是否已退出、最近的输出、临时数据目录、等页面出现的函数、停止并清理的函数。
 */
export function startDevApp({ port, romPath, windowPlacement }) {
  const packageName = JSON.parse(readFileSync(join(projectRoot, 'package.json'), 'utf8')).name;
  if (packageName !== 'ember' || !existsSync(join(projectRoot, 'forge.config.mts'))) throw new Error(`不是 Ember 项目目录：${projectRoot}`);
  if (!existsSync(romPath)) throw new Error(`找不到测试 ROM：${romPath}`);

  const userDataDirectory = mkdtempSync(join(tmpdir(), 'ember-dev-'));
  if (windowPlacement !== undefined) {
    mkdirSync(join(userDataDirectory, 'config'), { recursive: true });
    writeFileSync(join(userDataDirectory, 'config', 'shell-state.json'), JSON.stringify({ window: { ...windowPlacement, maximized: false }, layout: { bottomPanelHeight: null } }));
  }
  const forgeBin = join(dirname(createRequire(import.meta.url).resolve('@electron-forge/cli/package.json')), 'dist', 'electron-forge.js');
  const forge = spawn(process.execPath, [forgeBin, 'start', '--', `--remote-debugging-port=${port}`, `--user-data-dir=${userDataDirectory}`, `--cartridge=${romPath}`], {
    cwd: projectRoot,
    stdio: ['ignore', 'pipe', 'pipe'],
    detached: process.platform !== 'win32',
    windowsHide: true
  });
  const handle = { pid: forge.pid, exited: false, output: '', userDataDirectory };
  forge.stdout.on('data', (chunk) => (handle.output = (handle.output + chunk).slice(-8000)));
  forge.stderr.on('data', (chunk) => (handle.output = (handle.output + chunk).slice(-8000)));
  forge.on('exit', () => (handle.exited = true));

  /** 等页面出现，返回页面的 CDP 目标；开发版意外退出或超时抛出异常。 */
  handle.waitForPage = async () => {
    const target = await waitFor(
      async () => {
        if (handle.exited) return 'exited';
        try {
          const targets = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
          return targets.find((item) => item.type === 'page' && !item.url.startsWith('devtools://'));
        } catch {
          return undefined;
        }
      },
      (value) => value !== undefined,
      STARTUP_TIMEOUT_MS,
      500
    );
    if (target === 'exited' || target === undefined) throw new Error(target === 'exited' ? '开发版意外退出' : '等不到页面');
    return target;
  };

  /** 结束自己启动的进程树并删除临时数据目录。 */
  handle.stop = () => {
    killTree(forge.pid);
    rmSync(userDataDirectory, { recursive: true, force: true, maxRetries: 5, retryDelay: 300 });
  };
  return handle;
}
