/*
 * ------------------------------------------------------------------------
 * 名称：smoke-run.mjs
 * 说明：Electron 冒烟脚本：启动开发版，让主进程直接加载一个测试 ROM，经 CDP 确认帧号在增长、渲染进程收到画面帧与输入端口能送达核心、页面重载与核心进程重启后端口重新建立、关闭窗口后应用干净退出，最后只结束自己启动的进程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用法：node tools/dev/smoke-run.mjs [--rom=相对 core/test/data 或绝对路径] [--port=9302] [--seconds=3] [--no-build] [--skip-core-kill]
 *       页面里临时注册的监听只在 CDP 会话里存在（Runtime.evaluate），不写进界面代码；用户数据目录是临时目录，不影响已打开的 Ember；
 *       只结束本脚本启动的 forge 进程树与它下面的核心进程（按 PID），不按进程名结束任何进程；退出码 0 表示全部检查通过。
 * ------------------------------------------------------------------------
 */

import { execFileSync, spawn, spawnSync } from 'node:child_process';
import { existsSync, mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { createRequire } from 'node:module';
import { tmpdir } from 'node:os';
import { dirname, isAbsolute, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');

/** 默认的测试 ROM：nestest，选单画面是静态的，按开始键会运行测试而改变画面。 */
const DEFAULT_ROM = 'nestest/nestest.nes';
/** 开始键的按键掩码：A、B、选择、开始…… 位 3。 */
const START_BUTTON = 0x08;
/** 等待开发版启动并出现页面的最长时间（毫秒）：首次要先构建主进程、预加载脚本与界面。 */
const STARTUP_TIMEOUT_MS = 180_000;
/** 等待其他条件的最长时间（毫秒）。 */
const WAIT_TIMEOUT_MS = 20_000;
/** 关闭窗口后应用应在这个时间内自行退出（毫秒）：不超过网关等待核心停止的超时（5 秒）。 */
const GRACEFUL_EXIT_LIMIT_MS = 4500;

/** 解析命令行：--名称=值 或 --名称。 */
function parseArguments(argv) {
  const options = new Map();
  for (const argument of argv) {
    const match = /^--([^=]+)(?:=(.*))?$/.exec(argument);
    if (match) options.set(match[1], match[2] ?? 'true');
  }
  return options;
}

const options = parseArguments(process.argv.slice(2));
const port = Number(options.get('port') ?? 9302);
const seconds = Number(options.get('seconds') ?? 3);
const romOption = options.get('rom') ?? DEFAULT_ROM;
const romPath = isAbsolute(romOption) ? romOption : join(root, 'core', 'test', 'data', romOption);

const results = [];
/** 记录一项检查。 */
function check(name, ok, detail = '') {
  results.push({ name, ok, detail });
  console.log(`${ok ? '通过' : '失败'}：${name}${detail ? `（${detail}）` : ''}`);
}

function sleep(milliseconds) {
  return new Promise((done) => setTimeout(done, milliseconds));
}

/** 轮询直到条件成立或超时，返回最后一次的值。 */
async function waitFor(produce, accept, timeout = WAIT_TIMEOUT_MS, interval = 100) {
  const deadline = Date.now() + timeout;
  let value = await produce();
  while (!accept(value) && Date.now() < deadline) {
    await sleep(interval);
    value = await produce();
  }
  return value;
}

// ---------------------------------------------------------------- 进程管理

/** 本脚本启动的进程：forge 主进程与它的输出。 */
let forge;
let forgeOutput = '';
let userDataDirectory;

/** 结束一个进程及其全部子进程（按 PID，Windows 用 taskkill /T）。 */
function killTree(pid) {
  if (pid === undefined) return;
  try {
    if (process.platform === 'win32') execFileSync('taskkill', ['/PID', String(pid), '/T', '/F'], { stdio: 'ignore' });
    else process.kill(-pid, 'SIGKILL');
  } catch {
    // 进程已经退出。
  }
}

/** 列出 forge 进程树下命令行含指定文字的进程 PID（Windows）。 */
function findDescendants(rootPid, commandLineMarker) {
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

function cleanup() {
  killTree(forge?.pid);
  if (userDataDirectory !== undefined) rmSync(userDataDirectory, { recursive: true, force: true, maxRetries: 5, retryDelay: 300 });
}

// ---------------------------------------------------------------- CDP

/** 一条到页面的 CDP 连接。 */
async function connectCdp(url) {
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

/** 页面里临时注册的监听：收到端口后记录帧、保存输入端口；只用于冒烟，只存在于本次会话。 */
const INSTALL_LISTENER = `(() => {
  if (window.__smoke) return true;
  const state = { portMessages: 0, frames: 0, width: 0, height: 0, format: '', pixelBytes: 0, lastFrame: 0, hash: 0, input: null };
  window.__smoke = state;
  window.addEventListener('message', (event) => {
    if (event.source !== window || !event.data || event.data.type !== 'ember.stream-ports' || event.ports.length !== 2) return;
    state.portMessages += 1;
    state.input = event.ports[1];
    event.ports[0].onmessage = (frameEvent) => {
      const message = frameEvent.data;
      if (!message || message.type !== 'frame') return;
      const words = new Uint32Array(message.pixels);
      let hash = 2166136261;
      for (let index = 0; index < words.length; index += 61) hash = Math.imul(hash ^ words[index], 16777619) >>> 0;
      state.frames += 1;
      state.width = message.width;
      state.height = message.height;
      state.format = message.format;
      state.pixelBytes = message.pixels.byteLength;
      state.lastFrame = message.frame;
      state.hash = hash;
    };
  });
  return true;
})()`;

// ---------------------------------------------------------------- 主流程

async function main() {
  const packageName = JSON.parse(readFileSync(join(root, 'package.json'), 'utf8')).name;
  if (packageName !== 'ember' || !existsSync(join(root, 'forge.config.mts'))) throw new Error(`不是 Ember 项目目录：${root}`);
  if (!existsSync(romPath)) throw new Error(`找不到测试 ROM：${romPath}`);

  if (!options.has('no-build')) {
    for (const script of ['schema', 'core:build']) {
      const run = spawnSync('npm', ['run', script], { cwd: root, shell: true, stdio: 'inherit' });
      if (run.status !== 0) throw new Error(`npm run ${script} 失败`);
    }
  }

  userDataDirectory = mkdtempSync(join(tmpdir(), 'ember-smoke-'));
  const forgeBin = join(dirname(createRequire(import.meta.url).resolve('@electron-forge/cli/package.json')), 'dist', 'electron-forge.js');
  console.log(`启动开发版：ROM=${romPath}，调试端口=${port}`);
  forge = spawn(process.execPath, [forgeBin, 'start', '--', `--remote-debugging-port=${port}`, `--user-data-dir=${userDataDirectory}`, `--cartridge=${romPath}`], {
    cwd: root,
    stdio: ['ignore', 'pipe', 'pipe'],
    detached: process.platform !== 'win32',
    windowsHide: true
  });
  forge.stdout.on('data', (chunk) => (forgeOutput = (forgeOutput + chunk).slice(-8000)));
  forge.stderr.on('data', (chunk) => (forgeOutput = (forgeOutput + chunk).slice(-8000)));
  let forgeExited = false;
  forge.on('exit', () => (forgeExited = true));

  // 等页面出现。
  const target = await waitFor(
    async () => {
      if (forgeExited) return 'exited';
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
  const page = await connectCdp(target.webSocketDebuggerUrl);
  await page.send('Runtime.enable');

  const bridgeReady = await waitFor(() => page.evaluate('typeof window.ember === "object"').catch(() => false), (value) => value === true);
  check('预加载脚本暴露了 window.ember', bridgeReady === true);

  // 沙箱与接口白名单。
  const exposure = await page.evaluate(`({
    noNode: typeof window.require === 'undefined' && typeof window.process === 'undefined' && typeof window.ipcRenderer === 'undefined',
    cartridgesLoadHidden: window.ember.core.cartridges.load === undefined,
    openExposed: typeof window.ember.main.cartridges.open === 'function',
    requestPorts: typeof window.ember.streams.requestPorts === 'function',
    keys: Object.keys(window.ember).sort().join(',')
  })`);
  check('渲染进程没有 Node 与 ipcRenderer', exposure.noNode);
  check('cartridges.load 不暴露给界面，cartridges.open 与 streams.requestPorts 已暴露', exposure.cartridgesLoadHidden && exposure.openExposed && exposure.requestPorts, exposure.keys);

  // 主进程加载卡带（--cartridge）：等状态变成通电运行。
  const loadedStatus = await waitFor(
    () => page.evaluate('window.ember.core.runtime.status({})').catch(() => undefined),
    (status) => status !== undefined && status.power === 'on' && status.mode === 'running'
  );
  check('主进程按 --cartridge 加载了卡带并上电运行', loadedStatus?.power === 'on' && loadedStatus?.mode === 'running', loadedStatus ? `${loadedStatus.machine} / ${loadedStatus.cartridge?.pluginId}` : '没有状态');
  check('runtime.status 返回卡带信息', loadedStatus?.cartridge?.pluginId === 'nrom' && loadedStatus?.cartridge?.host === 'fc' && String(loadedStatus?.cartridge?.path).toLowerCase().endsWith('.nes'));

  // 帧号增长。
  const first = await page.evaluate('window.ember.core.runtime.status({})');
  await sleep(seconds * 1000);
  const second = await page.evaluate('window.ember.core.runtime.status({})');
  const frameDelta = second.frame - first.frame;
  check('runtime.status 的帧号在增长', frameDelta > 0, `${first.frame} → ${second.frame}，约 ${(frameDelta / seconds).toFixed(1)} 帧/秒`);
  check('帧率接近 NTSC 的 60.1', frameDelta / seconds > 45 && frameDelta / seconds < 66);
  check('周期数在增长', second.cycles > first.cycles);

  // 端口与帧。
  await page.evaluate(INSTALL_LISTENER);
  await page.evaluate('window.ember.streams.requestPorts()');
  const afterPorts = await waitFor(() => page.evaluate('window.__smoke'), (state) => state.frames > 0);
  check('渲染进程收到了端口', afterPorts.portMessages === 1);
  check('渲染进程收到了画面帧', afterPorts.frames > 0, `尺寸 ${afterPorts.width}x${afterPorts.height}，格式 ${afterPorts.format}，像素 ${afterPorts.pixelBytes} 字节`);
  check('帧尺寸为 256x240，像素格式为 rgba8，缓冲大小匹配', afterPorts.width === 256 && afterPorts.height === 240 && afterPorts.format === 'rgba8' && afterPorts.pixelBytes === 256 * 240 * 4);
  const received0 = (await page.evaluate('window.__smoke')).frames;
  await sleep(seconds * 1000);
  const received1 = await page.evaluate('window.__smoke');
  const receivedRate = (received1.frames - received0) / seconds;
  check('渲染进程每秒收到的帧数接近 60', receivedRate > 40 && receivedRate < 70, `${receivedRate.toFixed(1)} 帧/秒`);
  check('帧号随帧递增', received1.lastFrame > afterPorts.lastFrame);

  // 输入：选单画面静态，按开始键运行测试，画面改变。
  const stable0 = (await page.evaluate('window.__smoke')).hash;
  await sleep(400);
  const stable1 = (await page.evaluate('window.__smoke')).hash;
  if (stable0 === stable1) {
    await page.evaluate(`window.__smoke.input.postMessage({ type: 'buttons', port: 0, buttons: ${START_BUTTON} })`);
    await sleep(300);
    await page.evaluate(`window.__smoke.input.postMessage({ type: 'buttons', port: 0, buttons: 0 })`);
    const changed = await waitFor(
      async () => (await page.evaluate('window.__smoke')).hash,
      (hash) => hash !== stable1,
      5000,
      100
    );
    check('输入端口的按键送达核心（按开始键后画面改变）', changed !== stable1);
  } else {
    check('输入端口的按键送达核心（按开始键后画面改变）', false, '选单画面不是静态的，无法判断');
  }
  // 校验：格式不对的输入被核心宿主丢弃，不影响运行。
  await page.evaluate(`window.__smoke.input.postMessage({ type: 'buttons', port: 9, buttons: 999 }); window.__smoke.input.postMessage('垃圾')`);
  const afterJunk = (await page.evaluate('window.__smoke')).frames;
  await sleep(500);
  check('非法输入被丢弃，画面继续', (await page.evaluate('window.__smoke')).frames > afterJunk);

  // 页面重载后重新建立端口。
  await page.send('Page.reload');
  await waitFor(() => page.evaluate('typeof window.ember === "object" && !window.__smoke').catch(() => false), (value) => value === true);
  await page.evaluate(INSTALL_LISTENER);
  await page.evaluate('window.ember.streams.requestPorts()');
  const afterReload = await waitFor(() => page.evaluate('window.__smoke'), (state) => state.frames > 5);
  check('页面重载后重新请求端口，画面帧继续到达', afterReload.portMessages === 1 && afterReload.frames > 5, `${afterReload.frames} 帧`);

  // 核心进程重启后重新建立端口：结束核心进程（forge 进程树下命令行含 NodeService 的进程，即 utilityProcess）。
  if (!options.has('skip-core-kill') && process.platform === 'win32') {
    const corePids = findDescendants(forge.pid, 'node.mojom.NodeService');
    check('找到了核心进程（utilityProcess）', corePids.length === 1, corePids.join(','));
    if (corePids.length === 1) {
      const portMessagesBefore = (await page.evaluate('window.__smoke')).portMessages;
      killTree(corePids[0]);
      const restarted = await waitFor(
        () => page.evaluate('window.__smoke').catch(() => undefined),
        (state) => state !== undefined && state.portMessages > portMessagesBefore,
        30_000,
        200
      );
      check('核心进程重启后主进程重新给页面端口', restarted?.portMessages > portMessagesBefore);
      const status = await waitFor(
        () => page.evaluate('window.ember.core.runtime.status({})').catch(() => undefined),
        (value) => value !== undefined && value.power === 'off',
        30_000,
        200
      );
      check('重启后的核心可以响应请求（新核心里没有卡带，电源为 off）', status?.power === 'off');
    }
  }

  // 正常退出：经浏览器端点关闭全部窗口，应用走 will-quit → 停止核心的流程自行退出（核心进程的停止超时是 5 秒，快于它说明没有靠超时强杀）。
  page.close();
  const browserTarget = await (await fetch(`http://127.0.0.1:${port}/json/version`)).json();
  const browser = await connectCdp(browserTarget.webSocketDebuggerUrl);
  const closedAt = Date.now();
  browser.send('Browser.close').catch(() => undefined);
  const exited = await waitFor(
    () => forgeExited,
    (value) => value === true,
    GRACEFUL_EXIT_LIMIT_MS * 3,
    100
  );
  const elapsed = Date.now() - closedAt;
  check('关闭窗口后应用自行退出，核心干净停止（不靠超时强杀）', exited === true && elapsed < GRACEFUL_EXIT_LIMIT_MS, `${elapsed} 毫秒`);
}

let failure;
try {
  await main();
} catch (error) {
  failure = error;
  console.error(`冒烟脚本出错：${error instanceof Error ? error.message : String(error)}`);
  if (forgeOutput) console.error(`开发版最近的输出：\n${forgeOutput}`);
} finally {
  cleanup();
}

const failed = results.filter((item) => !item.ok);
console.log(`\n冒烟结果：${results.length - failed.length}/${results.length} 项通过${failure ? '，脚本中途出错' : ''}`);
process.exit(failed.length === 0 && failure === undefined ? 0 : 1);
