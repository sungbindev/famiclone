/*
 * ------------------------------------------------------------------------
 * 名称：smoke-run.mjs
 * 说明：Electron 冒烟脚本：启动开发版，让主进程直接加载一个测试 ROM，经 CDP 确认帧号在增长、运行页的渲染 Worker 收到并画出画面帧（读 Worker 的诊断）、画面非空、画面区按键能送达核心并改变画面、页面重载与核心进程重启后端口重新建立、关闭窗口后应用干净退出，最后只结束自己启动的进程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用法：node tools/dev/smoke-run.mjs [--rom=相对 core/test/data 或绝对路径] [--port=9302] [--seconds=3] [--no-build] [--skip-core-kill] [--screenshots=目录]
 *       画面端口由运行页自己请求并转交给渲染 Worker；脚本只在页面里临时挂一个 message 监听来数端口消息（Page.addScriptToEvaluateOnNewDocument，不写进界面代码），
 *       读 Worker 诊断用开发期才有的 window.emberRuntimeDiagnostics；按键用 CDP 的 Input.dispatchKeyEvent 发到画面区；用户数据目录是临时目录，不影响已打开的 Ember；
 *       只结束本脚本启动的 forge 进程树与它下面的核心进程（按 PID），不按进程名结束任何进程；退出码 0 表示全部检查通过。
 * ------------------------------------------------------------------------
 */

import { spawnSync } from 'node:child_process';
import { mkdirSync } from 'node:fs';
import { isAbsolute, join } from 'node:path';
import { connectCdp, findDescendants, killTree, parseArguments, pressKey, projectRoot as root, saveScreenshot, sleep, startDevApp, waitFor } from './dev-support.mjs';

/** 默认的测试 ROM：nestest，选单画面是静态的，按开始键（Enter）会运行测试而改变画面。 */
const DEFAULT_ROM = 'nestest/nestest.nes';
/** 关闭窗口后应用应在这个时间内自行退出（毫秒）：不超过网关等待核心停止的超时（5 秒）。 */
const GRACEFUL_EXIT_LIMIT_MS = 4500;

const options = parseArguments(process.argv.slice(2));
const port = Number(options.get('port') ?? 9302);
const seconds = Number(options.get('seconds') ?? 3);
const romOption = options.get('rom') ?? DEFAULT_ROM;
const romPath = isAbsolute(romOption) ? romOption : join(root, 'core', 'test', 'data', romOption);
/** 截图保存的目录（可选）：--screenshots=目录；截图只放在临时目录，不进仓库。 */
const screenshotDirectory = options.get('screenshots');
if (screenshotDirectory !== undefined) mkdirSync(screenshotDirectory, { recursive: true });

const results = [];
/** 记录一项检查。 */
function check(name, ok, detail = '') {
  results.push({ name, ok, detail });
  console.log(`${ok ? '通过' : '失败'}：${name}${detail ? `（${detail}）` : ''}`);
}

/** 本脚本启动的开发版。 */
let app;

function cleanup() {
  app?.stop();
}

/**
 * 页面里临时注册的监听：只数收到的端口消息、记下输入端口（用来发非法输入）；画面端口由运行页自己转交给渲染 Worker，不碰它。
 * 只用于冒烟，只存在于本次会话（页面重载后由 Page.addScriptToEvaluateOnNewDocument 重新装上）。
 */
const INSTALL_LISTENER = `(() => {
  if (window.__smoke) return true;
  const state = { portMessages: 0, input: null };
  window.__smoke = state;
  window.addEventListener('message', (event) => {
    if (event.source !== window || !event.data || event.data.type !== 'ember.stream-ports' || event.ports.length !== 2) return;
    state.portMessages += 1;
    state.input = event.ports[1];
  });
  return true;
})()`;

/** 读渲染 Worker 的诊断；reset 为 true 时先清空统计。 */
const readDiagnostics = (reset = false) => `window.emberRuntimeDiagnostics.read(${reset})`;

// ---------------------------------------------------------------- 主流程

async function main() {
  if (!options.has('no-build')) {
    for (const script of ['schema', 'core:build']) {
      const run = spawnSync('npm', ['run', script], { cwd: root, shell: true, stdio: 'inherit' });
      if (run.status !== 0) throw new Error(`npm run ${script} 失败`);
    }
  }

  console.log(`启动开发版：ROM=${romPath}，调试端口=${port}`);
  app = startDevApp({ port, romPath });
  const target = await app.waitForPage();
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

  // 端口与帧：运行页自己请求端口、把画面端口转交给渲染 Worker；这里只读 Worker 的诊断，并临时监听窗口消息数端口。
  await page.send('Page.enable');
  await page.send('Page.addScriptToEvaluateOnNewDocument', { source: INSTALL_LISTENER });
  await page.evaluate(INSTALL_LISTENER);
  const diagnosticsReady = await waitFor(() => page.evaluate('typeof window.emberRuntimeDiagnostics === "object"').catch(() => false), (value) => value === true);
  check('运行页暴露了开发期的 Worker 诊断', diagnosticsReady === true);
  await page.evaluate('window.ember.streams.requestPorts()');
  const afterPorts = await waitFor(
    async () => ({ ports: await page.evaluate('window.__smoke.portMessages'), diagnostics: await page.evaluate(readDiagnostics()) }),
    (value) => value.ports >= 1 && value.diagnostics.received > 0
  );
  check('渲染进程收到了端口', afterPorts.ports >= 1);
  check('渲染 Worker 初始化了 WebGL 并收到画面帧', afterPorts.diagnostics.rendererReady === true && afterPorts.diagnostics.received > 0, `已收到 ${afterPorts.diagnostics.received} 帧，画布 ${afterPorts.diagnostics.canvasWidth}x${afterPorts.diagnostics.canvasHeight}`);
  check('帧尺寸为 256x240（像素格式与字节数通过 Worker 的校验，没有无效帧）', afterPorts.diagnostics.frameWidth === 256 && afterPorts.diagnostics.frameHeight === 240 && afterPorts.diagnostics.invalid === 0);
  // 从这里开始统计一段时间。
  await page.evaluate(readDiagnostics(true));
  await sleep(seconds * 1000);
  const window1 = await page.evaluate(readDiagnostics());
  check('渲染进程每秒收到的帧数接近 60', window1.receivedFps > 40 && window1.receivedFps < 70, `${window1.receivedFps.toFixed(1)} 帧/秒`);
  check('渲染 Worker 每秒画出的帧数接近 60', window1.renderedFps > 40 && window1.renderedFps < 70, `${window1.renderedFps.toFixed(1)} 帧/秒，丢弃 ${window1.dropped} 帧`);
  check('核心宿主没有因背压丢帧', window1.hostDropped === 0 && window1.coreMissed === 0, `宿主丢弃 ${window1.hostDropped}，帧号缺口 ${window1.coreMissed}`);
  check('帧号随帧递增', window1.lastFrame > afterPorts.diagnostics.lastFrame, `${afterPorts.diagnostics.lastFrame} → ${window1.lastFrame}`);
  const picture0 = await page.evaluate(readDiagnostics());
  check('渲染 Worker 收到的画面非空（有非黑像素）', picture0.lastFrameNonBlackRatio > 0.001, `非黑像素 ${(picture0.lastFrameNonBlackRatio * 100).toFixed(1)}%`);
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-menu.png'));

  // 输入：画面区获得焦点后按 Enter（开始键），选单画面是静态的，按下后运行测试，画面改变。
  const stable0 = (await page.evaluate(readDiagnostics())).lastFrameHash;
  await sleep(400);
  const stable1 = (await page.evaluate(readDiagnostics())).lastFrameHash;
  await page.evaluate('document.querySelector(\'[role="application"]\').focus()');
  if (stable0 === stable1) {
    await pressKey(page, { key: 'Enter', code: 'Enter', keyCode: 13 }, 300);
    const changed = await waitFor(
      async () => (await page.evaluate(readDiagnostics())).lastFrameHash,
      (hash) => hash !== stable1,
      5000,
      100
    );
    check('键盘输入送达核心（画面区按 Enter 后画面改变）', changed !== stable1);
  } else {
    check('键盘输入送达核心（画面区按 Enter 后画面改变）', false, '选单画面不是静态的，无法判断');
  }
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-after-enter.png'));
  // 校验：格式不对的输入被核心宿主丢弃，不影响运行；格式不对的帧也不会进来（Worker 一侧的校验由单元测试覆盖）。
  await page.evaluate(`window.__smoke.input.postMessage({ type: 'buttons', port: 9, buttons: 999 }); window.__smoke.input.postMessage('垃圾')`);
  const afterJunk = (await page.evaluate(readDiagnostics())).received;
  await sleep(500);
  check('非法输入被丢弃，画面继续', (await page.evaluate(readDiagnostics())).received > afterJunk);

  // 页面重载后：运行页重新请求端口，画面帧继续到达。
  await page.send('Page.reload');
  await waitFor(() => page.evaluate('typeof window.ember === "object" && typeof window.emberRuntimeDiagnostics === "object"').catch(() => false), (value) => value === true);
  const afterReload = await waitFor(
    async () => ({ ports: await page.evaluate('window.__smoke.portMessages').catch(() => 0), diagnostics: await page.evaluate(readDiagnostics()).catch(() => ({ received: 0 })) }),
    (value) => value.ports >= 1 && value.diagnostics.received > 5
  );
  check('页面重载后运行页重新请求端口，画面帧继续到达', afterReload.ports >= 1 && afterReload.diagnostics.received > 5, `${afterReload.diagnostics.received} 帧`);

  // 核心进程重启后重新建立端口：结束核心进程（forge 进程树下命令行含 NodeService 的进程，即 utilityProcess）。
  if (!options.has('skip-core-kill') && process.platform === 'win32') {
    const corePids = findDescendants(app.pid, 'node.mojom.NodeService');
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
    () => app.exited,
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
  if (app?.output) console.error(`开发版最近的输出：\n${app.output}`);
} finally {
  cleanup();
}

const failed = results.filter((item) => !item.ok);
console.log(`\n冒烟结果：${results.length - failed.length}/${results.length} 项通过${failure ? '，脚本中途出错' : ''}`);
process.exit(failed.length === 0 && failure === undefined ? 0 : 1);
