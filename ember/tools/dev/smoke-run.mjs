/*
 * ------------------------------------------------------------------------
 * 名称：smoke-run.mjs
 * 说明：Electron 冒烟脚本：启动开发版，让主进程直接加载一个测试 ROM，经 CDP 确认帧号在增长、运行页的渲染 Worker 收到并画出画面帧（读 Worker 的诊断）、画面非空、画面区按键能送达核心并改变画面、 *       键盘独占的接口与原生模块能加载、开启、关闭（不注入系统按键）、 *       系统栏左侧的运行按钮（只有图标）实时显示运行状态并展开运行面板、面板按钮与快捷键（F8）能暂停和继续、“文件”菜单的“最近打开”与“选择机型”子菜单、
 *       启动时底部面板关闭且没有活动栏、边栏、标签栏与右边栏、展开与收起面板时窗口大小不变而画面框等比缩放并留在显示区内、画面框的宽高比、最小窗口下标题栏不挤压不换行并留出拖动空白、
 *       首选项对话框（独立的系统窗口，主题与主窗口一致）与首选项（出厂默认值、非法取值被拒绝、画面比例与“显示全部 240 行”改后画面框跟着变、主题切换、音量保存、恢复默认）、调试服务与系统栏右侧的“调试模式”、
 *       页面重载与核心进程重启后端口重新建立且运行按钮状态重新读取、关闭窗口后应用干净退出，最后只结束自己启动的进程。
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
import { existsSync, mkdirSync, readFileSync } from 'node:fs';
import { isAbsolute, join } from 'node:path';
import { connectCdp, findDescendants, killTree, parseArguments, pressKey, projectRoot as root, saveScreenshot, sleep, startDevApp, waitFor } from './dev-support.mjs';

/** 默认的测试 ROM：nestest，选单画面是静态的，按开始键（Enter）会运行测试而改变画面。 */
const DEFAULT_ROM = 'nestest/nestest.nes';
/** 关闭窗口后应用应在这个时间内自行退出（毫秒）：不超过网关等待核心停止的超时（5 秒）。 */
const GRACEFUL_EXIT_LIMIT_MS = 4500;
/** 标题栏菜单与系统窗口按钮之间至少留的可拖动空白（像素）。 */
const MIN_DRAG_GAP = 48;
/** 画面框的显示宽高比：256×240 的整帧、像素宽高比 8:7。 */
const FRAME_RATIO = 128 / 105;
/** 预置的窗口位置：窗口大小每次启动都是最小尺寸，这里只固定位置。 */
const START_PLACEMENT = { x: 400, y: 60 };
/** 调试服务的令牌（启动参数 --debug-token）。 */
const DEBUG_TOKEN = 'smoke-token';

/** 比较“当前取值是否等于出厂默认值”时跳过令牌：令牌的默认值是空，核心启动时自动生成并保存。 */
const comparable = (setting) => setting.id !== 'debug.token';

const options = parseArguments(process.argv.slice(2));
const port = Number(options.get('port') ?? 9302);
const debugPort = Number(options.get('debug-port') ?? 5699);
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
/** 到主窗口页面的 CDP 连接。 */
let page;

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

/** 窗口大小、显示区与画面框的几何信息（CSS 像素）以及底部面板是否关闭。 */
const GEOMETRY = `(() => {
  const frame = document.querySelector('.ember-run-frame').getBoundingClientRect();
  const area = document.querySelector('.ui-app-frame__page').getBoundingClientRect();
  const bottom = document.querySelector('.ui-app-frame__bottom');
  return {
    innerWidth: window.innerWidth, innerHeight: window.innerHeight,
    frameWidth: frame.width, frameHeight: frame.height,
    margins: { left: frame.left - area.left, right: area.right - frame.right, top: frame.top - area.top, bottom: area.bottom - frame.bottom },
    bottomHidden: bottom === null || bottom.hidden
  };
})()`;

/** 标题栏各部分的位置与宽度，以及菜单是否被挤压（文字被截断或折行）。 */
const TITLE_BAR_METRICS = `(() => {
  const rect = (selector) => { const box = document.querySelector(selector).getBoundingClientRect(); return { left: box.left, right: box.right, width: box.width }; };
  const bar = document.querySelector('.ui-title-bar');
  const barStyle = getComputedStyle(bar);
  const items = [...document.querySelectorAll('.ui-title-bar__menus [role="menuitem"]')];
  const tops = new Set(items.map((item) => Math.round(item.getBoundingClientRect().top)));
  return {
    innerWidth: window.innerWidth, innerHeight: window.innerHeight,
    paddingLeft: parseFloat(barStyle.paddingLeft), paddingRight: parseFloat(barStyle.paddingRight),
    brand: rect('.ui-title-bar__brand'), menus: rect('.ui-title-bar__menus'),
    brandMargin: parseFloat(getComputedStyle(document.querySelector('.ui-title-bar__brand')).marginRight),
    menuCount: items.length, menuRows: tops.size, menuClipped: items.some((item) => item.scrollWidth > item.clientWidth + 1),
    overlay: navigator.windowControlsOverlay ? navigator.windowControlsOverlay.getTitlebarAreaRect().width : -1
  };
})()`;

/** 点系统栏里指定名称的按钮（运行、设备）。 */
async function clickBarButton(label) {
  return page.evaluate(`(() => { const button = document.querySelector('.ui-status-bar [aria-label=' + JSON.stringify(${JSON.stringify(label)}) + ']'); if (!button || button.disabled) return false; button.click(); return true; })()`);
}

/** 点系统栏里指定名称的按钮，等布局稳定后读几何信息。 */
async function toggleAndMeasure(label) {
  await clickBarButton(label);
  await sleep(500);
  return page.evaluate(GEOMETRY);
}

/** 画面框是否完整地在显示区内（允许 1 像素的取整误差），且宽高比仍是 128:105。 */
function frameFits(geometry) {
  const inside = Object.values(geometry.margins).every((margin) => margin >= -1);
  return inside && Math.abs(geometry.frameWidth / geometry.frameHeight - FRAME_RATIO) < 0.02;
}

/** 窗口大小是否与 before 一致（误差不超过 1 像素）。 */
function sameWindowSize(before, after) {
  return Math.abs(before.innerWidth - after.innerWidth) <= 1 && Math.abs(before.innerHeight - after.innerHeight) <= 1;
}

/** 页面里第一个匹配元素的文字；没有该元素时为空字符串。 */
async function textOf(selector) {
  return page.evaluate(`(document.querySelector(${JSON.stringify(selector)})?.textContent ?? '')`);
}

/** 点底部面板里指定文字的按钮；按钮不存在或置灰时返回 false。 */
async function clickPanelButton(name) {
  return page.evaluate(`(() => { const button = [...document.querySelectorAll('.ember-shell-panel button')].find((item) => item.textContent.trim() === ${JSON.stringify(name)}); if (!button || button.disabled) return false; button.click(); return true; })()`);
}

/** 点菜单栏里的一个菜单名（打开，再点一次关闭）。 */
async function menuButton(label) {
  return page.evaluate(`(() => { const button = [...document.querySelectorAll('[role="menubar"] [role="menuitem"]')].find((item) => item.textContent.startsWith(${JSON.stringify(label)})); button?.click(); return Boolean(button); })()`);
}

/** 某个菜单弹层里的全部菜单项：名称、是否置灰、是否选中（单选项）、完整文字。 */
async function menuItems(popupSelector) {
  return page.evaluate(`[...document.querySelectorAll(${JSON.stringify(`${popupSelector} [role^="menuitem"]`)})].map((item) => ({ label: item.querySelector('.ui-menu__label')?.textContent ?? '', disabled: item.getAttribute('aria-disabled') === 'true', checked: item.getAttribute('aria-checked'), text: item.textContent }))`);
}

/** 把指针移到菜单项上（有子菜单的项随之展开）。 */
async function hoverMenuItem(label) {
  return page.evaluate(`(() => { const item = [...document.querySelectorAll('.ui-menu__popup [role^="menuitem"]')].find((element) => element.querySelector('.ui-menu__label')?.textContent === ${JSON.stringify(label)}); item?.dispatchEvent(new MouseEvent('mousemove', { bubbles: true })); return Boolean(item); })()`);
}

// ---------------------------------------------------------------- 主流程

/**
 * 连接调试服务（Node 自带的 WebSocket）：返回 call（发 JSON-RPC 请求并等响应）、收到的通知、关闭码。
 * @param {string} url 端点文件里的地址。
 */
function openDebugSocket(url) {
  return new Promise((resolvePromise, rejectPromise) => {
    const socket = new WebSocket(url);
    socket.binaryType = 'arraybuffer';
    const pending = new Map();
    let nextId = 1;
    const client = { notifications: [], closeCode: undefined, closed: false };
    client.call = (method, params = {}) =>
      new Promise((resolveCall) => {
        const id = nextId++;
        pending.set(id, resolveCall);
        socket.send(JSON.stringify({ jsonrpc: '2.0', id, method, params }));
      });
    client.close = () => socket.close();
    socket.addEventListener('message', (event) => {
      let message;
      if (typeof event.data === 'string') {
        message = JSON.parse(event.data);
      } else {
        // 二进制帧：4 字节大端长度 + JSON 报文 + data 的原始字节。
        const bytes = new Uint8Array(event.data);
        const jsonLength = new DataView(event.data).getUint32(0, false);
        message = JSON.parse(new TextDecoder().decode(bytes.subarray(4, 4 + jsonLength)));
        message.binary = bytes.subarray(4 + jsonLength);
      }
      if (message.id !== undefined && pending.has(message.id)) {
        pending.get(message.id)(message);
        pending.delete(message.id);
      } else if (message.method !== undefined) {
        client.notifications.push(message);
      }
    });
    socket.addEventListener('open', () => resolvePromise(client));
    socket.addEventListener('error', () => rejectPromise(new Error(`无法连接调试服务：${url}`)));
    socket.addEventListener('close', (event) => {
      client.closeCode = event.code;
      client.closed = true;
    });
  });
}

/** 读端点文件；还没有时为 undefined。 */
function readEndpoint() {
  const file = join(app.userDataDirectory, 'run', 'debug-endpoint.json');
  if (!existsSync(file)) return undefined;
  try {
    return JSON.parse(readFileSync(file, 'utf8'));
  } catch {
    return undefined;
  }
}

async function main() {
  if (!options.has('no-build')) {
    for (const script of ['schema', 'core:build']) {
      const run = spawnSync('npm', ['run', script], { cwd: root, shell: true, stdio: 'inherit' });
      if (run.status !== 0) throw new Error(`npm run ${script} 失败`);
    }
  }

  console.log(`启动开发版：ROM=${romPath}，调试端口=${port}`);
  app = startDevApp({ port, romPath, windowPlacement: START_PLACEMENT, extraArguments: [`--debug-port=${debugPort}`, `--debug-token=${DEBUG_TOKEN}`] });
  const target = await app.waitForPage();
  page = await connectCdp(target.webSocketDebuggerUrl);
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

  // 输入：画面区获得焦点后按 Enter，nrom 的脚本把它设为手柄 1 的开始键；选单画面是静态的，按下后运行测试，画面改变。
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
  await page.evaluate(`window.__smoke.input.postMessage({ type: 'key', code: 'bad code', down: true }); window.__smoke.input.postMessage({ type: 'mouse', kind: 'drag' }); window.__smoke.input.postMessage({ type: 'buttons', port: 9, buttons: 999 }); window.__smoke.input.postMessage('垃圾')`);
  const afterJunk = (await page.evaluate(readDiagnostics())).received;
  await sleep(500);
  check('非法输入被丢弃，画面继续', (await page.evaluate(readDiagnostics())).received > afterJunk);

  // 指针锁定：“点击画面捕获鼠标”的前提。临时在画面区挂一个 pointerdown 去请求锁定，用真实的鼠标按下触发，确认主进程的权限策略放行 pointerLock。
  await page.evaluate(`(() => { window.__lock = 'pending'; const screen = document.querySelector('.ember-run-screen'); document.addEventListener('pointerlockchange', () => { window.__lock = document.pointerLockElement ? 'locked' : 'unlocked'; }); document.addEventListener('pointerlockerror', () => { window.__lock = 'error'; }); screen.addEventListener('pointerdown', () => screen.requestPointerLock(), { once: true }); })()`);
  const lockPoint = await page.evaluate(`(() => { const box = document.querySelector('.ember-run-screen').getBoundingClientRect(); return { x: box.left + box.width / 2, y: box.top + box.height / 2 }; })()`);
  await page.send('Page.bringToFront');
  await page.send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: lockPoint.x, y: lockPoint.y });
  await page.send('Input.dispatchMouseEvent', { type: 'mousePressed', x: lockPoint.x, y: lockPoint.y, button: 'left', clickCount: 1 });
  await page.send('Input.dispatchMouseEvent', { type: 'mouseReleased', x: lockPoint.x, y: lockPoint.y, button: 'left', clickCount: 1 });
  const lockState = await waitFor(() => page.evaluate('window.__lock'), (value) => value !== 'pending', 3000, 100);
  check('点画面可以锁定指针（权限策略只放行 pointerLock）', lockState === 'locked', String(lockState));
  await page.evaluate('document.exitPointerLock()');

  // 系统栏与底部面板：运行按钮（系统栏左侧，只有图标）的图标与提示实时显示运行状态，点它展开运行面板（控制按钮、声音），再经菜单与快捷键操作。
  const runButtonInfo = () => page.evaluate(`(() => { const button = document.querySelector('.ui-status-bar [aria-label="运行"]'); return button ? { title: button.title, tone: button.getAttribute('data-tone'), text: button.textContent, path: button.querySelector('path')?.getAttribute('d') ?? '' } : null; })()`);
  const runningButton = await runButtonInfo();
  check('系统栏左侧的运行按钮只有图标，运行中时提示是“运行：运行中”、图标为绿色', runningButton?.title === '运行：运行中' && runningButton?.tone === 'success' && runningButton?.text === '', JSON.stringify(runningButton));
  const barText = await textOf('.ui-status-bar');
  check('系统栏显示主机型号、卡带名与 mapper 插件，不显示电源状态与音量', ['经典 FC', 'nestest.nes', 'NROM'].every((word) => barText.includes(word)) && !['运行中', '音量'].some((word) => barText.includes(word)), barText.replace(/\s+/g, ' ').slice(0, 60));
  check('点系统栏的运行按钮', await clickBarButton('运行'));
  const panelText = await waitFor(() => textOf('.ember-shell-panel'), (text) => text.includes('软复位'), 5000, 100);
  check('运行面板只有控制按钮与声音，不显示主机型号、卡带、mapper 插件与“状态”', ['断电', '暂停', '软复位', '硬复位'].every((word) => panelText.includes(word)) && !['主机型号', '经典 FC', 'nestest.nes', 'NROM', '状态'].some((word) => panelText.includes(word)), panelText.replace(/\s+/g, ' ').slice(0, 60));
  const panelHeader = await page.evaluate(`(() => { const title = document.querySelector('.ember-shell-panel__title'); return { title: title?.textContent, weight: Number(getComputedStyle(title).fontWeight), close: Boolean(document.querySelector('.ember-shell-panel__header button')) }; })()`);
  check('面板标题行：左边是加粗的面板标题“运行”，右边有关闭按钮', panelHeader.title === '运行' && panelHeader.weight >= 600 && panelHeader.close, JSON.stringify(panelHeader));
  check('运行面板的“暂停”按钮可点击', await clickPanelButton('暂停'));
  const paused = await waitFor(() => page.evaluate('window.ember.core.runtime.status({})'), (value) => value.mode === 'paused');
  const pausedHint = await waitFor(() => textOf('.ember-run-overlay'), (text) => text.includes('已暂停'));
  check('点“暂停”后核心暂停，画面上出现“已暂停”提示', paused.mode === 'paused' && pausedHint.includes('已暂停'));
  const pausedButton = await runButtonInfo();
  check('暂停后运行按钮的图标换成暂停图标、不再是绿色，提示是“运行：已暂停”', pausedButton?.title === '运行：已暂停' && pausedButton?.tone === 'normal' && pausedButton?.path !== runningButton?.path, JSON.stringify(pausedButton));
  await sleep(300);
  check('面板操作结束后焦点回到画面', await page.evaluate('document.activeElement?.getAttribute("role") === "application"'));
  check('画面区持有焦点时不画焦点框', await page.evaluate('getComputedStyle(document.activeElement).outlineStyle === "none"'));
  await pressKey(page, { key: 'F8', code: 'F8', keyCode: 119 }, 50);
  const resumed = await waitFor(() => page.evaluate('window.ember.core.runtime.status({})'), (value) => value.mode === 'running');
  check('画面区收到页面内的 F8 键盘事件时继续运行（没有键盘独占的发送方式，F 键不被画面区占用），提示消失', resumed.mode === 'running' && (await waitFor(() => textOf('.ember-run-overlay'), (text) => text === '')) === '');
  // 键盘独占：只调用接口核对原生模块能加载、能开启与关闭（脚本不注入系统按键，避免按键发到别的程序）；最后恢复开启，与界面记录的状态一致。
  const exclusive = await page.evaluate(
    `(async () => { const shell = window.ember.shell; const methods = ['setKeyboardCapture', 'onNativeKey', 'onKeyboardReleased'].every((name) => typeof shell[name] === 'function'); const off = await shell.setKeyboardCapture(false); const on = await shell.setKeyboardCapture(true); return { methods, off, on }; })()`
  );
  check('预加载脚本暴露了键盘独占的接口', exclusive.methods === true);
  check(
    process.platform === 'win32' ? '键盘独占的原生模块已加载：关闭返回 false、开启返回 true' : '不支持键盘独占的平台：开关都返回 false，继续用页面内的键盘事件',
    exclusive.off === false && exclusive.on === (process.platform === 'win32'),
    JSON.stringify(exclusive)
  );
  check('继续运行后运行按钮回到绿色三角形', (await waitFor(runButtonInfo, (info) => info?.title === '运行：运行中', 3000, 100))?.path === runningButton?.path);
  check('再点运行按钮收起面板', (await clickBarButton('运行')) && (await waitFor(() => page.evaluate('document.querySelector(".ui-app-frame__bottom") === null'), (value) => value === true, 3000, 100)) === true);
  await menuButton('文件');
  const fileItems = await menuItems('.ui-menu__popup:not(.ui-menu__popup--sub)');
  check(
    '“文件”菜单：选择机型、打开卡带、最近打开（启动参数加载的卡带已记入）、关闭卡带（有卡带可用）、退出，都没有三个点',
    fileItems.map((entry) => entry.label).join('|') === '选择机型|打开卡带|最近打开|关闭卡带|退出' && fileItems.every((entry) => !entry.disabled),
    JSON.stringify(fileItems)
  );
  await hoverMenuItem('选择机型');
  const machineItems = await waitFor(() => menuItems('.ui-menu__popup--sub'), (items) => items.length > 0, 3000, 100);
  check(
    '“选择机型”子菜单：经典 FC 可用并打对勾；小霸王 2000 没有 BIOS 置灰并注明“未添加 BIOS”，其余三个机型置灰并注明“暂未支持”',
    machineItems.length === 5 &&
      !machineItems[0].disabled &&
      machineItems[0].checked === 'true' &&
      machineItems[2].disabled &&
      machineItems[2].text.includes('未添加 BIOS') &&
      [1, 3, 4].every((index) => machineItems[index].disabled && machineItems[index].text.includes('暂未支持')),
    JSON.stringify(machineItems.map((entry) => entry.label))
  );
  await hoverMenuItem('最近打开');
  const recentItems = await waitFor(() => menuItems('.ui-menu__popup--sub'), (items) => items.length > 0 && items[0].label === 'nestest.nes', 3000, 100);
  check('“最近打开”子菜单列出启动参数加载的卡带（只给显示名，不含路径）', recentItems.length === 1 && recentItems[0].label === 'nestest.nes' && !recentItems[0].text.includes('\\') && !recentItems[0].text.includes('/'), JSON.stringify(recentItems));
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-menu-recent.png'));
  await menuButton('文件');

  // 面板与窗口：启动时面板是关闭的，窗口是最小尺寸；先在最小窗口下核对标题栏与画面框，再展开面板核对窗口大小不变、画面框等比缩小并留在显示区内。
  const initial = await page.evaluate(GEOMETRY);
  check('启动时底部面板是关闭的，没有活动栏、边栏、标签栏与右边栏', initial.bottomHidden && (await page.evaluate(`!document.querySelector('.ui-activity-bar, .ui-app-frame__side, .ui-app-frame__right, [role="tablist"]')`)), JSON.stringify({ bottomHidden: initial.bottomHidden }));
  const tiny = await page.evaluate(TITLE_BAR_METRICS);
  const dragGap = tiny.innerWidth - tiny.paddingRight - tiny.menus.right;
  console.log(`最小窗口 ${tiny.innerWidth}×${tiny.innerHeight}：图标与名称 ${tiny.brand.width.toFixed(1)}，菜单 ${tiny.menus.left.toFixed(1)}–${tiny.menus.right.toFixed(1)}（宽 ${tiny.menus.width.toFixed(1)}），系统按钮从 ${(tiny.innerWidth - tiny.paddingRight).toFixed(1)} 起，菜单到系统按钮的空白 ${dragGap.toFixed(1)}`);
  check('最小窗口下菜单都在同一行、文字没有被截断', tiny.menuCount > 0 && tiny.menuRows === 1 && !tiny.menuClipped, `${tiny.menuCount} 个菜单，${tiny.menuRows} 行`);
  check(`最小窗口下菜单与系统窗口按钮之间至少有 ${MIN_DRAG_GAP} 像素可拖动的空白`, dragGap >= MIN_DRAG_GAP, `${dragGap.toFixed(1)}`);
  check('画面框的宽高比是 128:105（8:7 的像素宽高比，不变形）', Math.abs(initial.frameWidth / initial.frameHeight - FRAME_RATIO) < 0.02, `${initial.frameWidth.toFixed(1)}×${initial.frameHeight.toFixed(1)}`);
  check('最小窗口下画面框四周的间距一样（误差不超过 2 像素）', Math.max(...Object.values(initial.margins)) - Math.min(...Object.values(initial.margins)) <= 2, JSON.stringify(initial.margins));
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-min-window.png'));
  const bottomOpen = await toggleAndMeasure('运行');
  check('展开运行面板：窗口大小不变，画面框等比缩小并留在显示区内', sameWindowSize(initial, bottomOpen) && frameFits(bottomOpen) && bottomOpen.frameHeight < initial.frameHeight, `窗口 ${bottomOpen.innerWidth}×${bottomOpen.innerHeight}，画面框 ${initial.frameWidth.toFixed(1)}×${initial.frameHeight.toFixed(1)} → ${bottomOpen.frameWidth.toFixed(1)}×${bottomOpen.frameHeight.toFixed(1)}`);
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-panels-open.png'));
  const allClosed = await toggleAndMeasure('运行');
  check('收起面板后窗口大小不变，画面框回到原来的大小', sameWindowSize(initial, allClosed) && Math.abs(allClosed.frameWidth - initial.frameWidth) <= 2 && Math.abs(allClosed.frameHeight - initial.frameHeight) <= 2, `画面框 ${allClosed.frameWidth.toFixed(1)}×${allClosed.frameHeight.toFixed(1)}`);

  // 首选项：核心登记的设置项与出厂默认值、非法取值被拒绝；改画面比例与“显示全部 240 行”后运行页的画面框立即跟着变；改主题后系统主题立即切换；调试服务改音量会保存到首选项；最后全部恢复默认。
  const listed = await page.evaluate('window.ember.core.preferences.list({})');
  const valueOf = (id) => listed.settings.find((setting) => setting.id === id)?.value;
  const expectedIds = ['appearance.theme', 'display.aspect', 'display.scaling', 'display.showAllLines', 'sound.buffer', 'sound.startMuted', 'sound.volume', 'input.captureMouse', 'printer.outputDirectory', 'debug.enabled', 'debug.port', 'debug.token', 'debug.outputDirectory'];
  // 令牌的默认值是空，核心启动时自动生成并保存，所以不与默认值比较，单独检查。
  check('首选项列出了登记的 13 个设置项，取值都是出厂默认值（令牌除外）', expectedIds.every((id) => listed.settings.some((setting) => setting.id === id && (!comparable(setting) || JSON.stringify(setting.value) === JSON.stringify(setting.defaultValue)))) && listed.settings.length === expectedIds.length, listed.settings.map((setting) => `${setting.id}=${setting.id === 'debug.token' ? '…' : setting.value}`).join('，'));
  check('调试服务的令牌已经自动生成（32 个字母与数字），默认端口 5688、默认不开启（启动参数强制开启不改首选项）', /^[A-Za-z0-9]{32}$/.test(String(valueOf('debug.token'))) && valueOf('debug.port') === 5688 && valueOf('debug.enabled') === false);
  check('出厂默认值：主题跟随系统、画面比例 8:7、适应窗口、显示全部行、缓冲标准、启动时不静音、音量 100', valueOf('appearance.theme') === 'system' && valueOf('display.aspect') === '8:7' && valueOf('display.scaling') === 'fit' && valueOf('display.showAllLines') === true && valueOf('sound.buffer') === 'standard' && valueOf('sound.startMuted') === false && valueOf('sound.volume') === 100);
  const invalid = await page.evaluate(`window.ember.core.preferences.set({ id: 'display.aspect', value: '16:9' }).then(() => 'accepted', (error) => error?.code ?? 'failed')`);
  check('不在可选范围内的取值被拒绝（preferences.invalidValue）', invalid === 'preferences.invalidValue', String(invalid));
  const unknown = await page.evaluate(`window.ember.core.preferences.set({ id: 'display.nothing', value: 1 }).then(() => 'accepted', (error) => error?.code ?? 'failed')`);
  check('没有登记的设置项被拒绝（preferences.unknown）', unknown === 'preferences.unknown', String(unknown));
  const ratioOf = async () => {
    await sleep(500);
    const geometry = await page.evaluate(GEOMETRY);
    return geometry.frameWidth / geometry.frameHeight;
  };
  await page.evaluate(`window.ember.core.preferences.set({ id: 'display.showAllLines', value: false })`);
  const croppedRatio = await ratioOf();
  check('关闭“显示全部 240 行”后画面框按 256x224 的 8:7 比例（64:49）', Math.abs(croppedRatio - 64 / 49) < 0.02, croppedRatio.toFixed(3));
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-cropped.png'));
  await page.evaluate(`window.ember.core.preferences.set({ id: 'display.aspect', value: '4:3' })`);
  const ratio43 = await ratioOf();
  check('画面比例改成 4:3 后画面框是 4:3', Math.abs(ratio43 - 4 / 3) < 0.02, ratio43.toFixed(3));
  await page.evaluate(`window.ember.core.preferences.reset({ id: 'display.aspect' })`);
  await page.evaluate(`window.ember.core.preferences.reset({ id: 'display.showAllLines' })`);
  const restoredRatio = await ratioOf();
  check('恢复默认后画面框回到 128:105', Math.abs(restoredRatio - FRAME_RATIO) < 0.02, restoredRatio.toFixed(3));
  await page.evaluate(`window.ember.core.preferences.set({ id: 'appearance.theme', value: 'light' })`);
  const lightApplied = await waitFor(() => page.evaluate(`window.matchMedia('(prefers-color-scheme: light)').matches`), (matches) => matches === true, 5000, 100);
  check('主题改成“浅色”后界面立即变成浅色', lightApplied === true);
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-light-theme.png'));
  await page.evaluate(`window.ember.core.preferences.set({ id: 'appearance.theme', value: 'dark' })`);
  const darkApplied = await waitFor(() => page.evaluate(`window.matchMedia('(prefers-color-scheme: dark)').matches`), (matches) => matches === true, 5000, 100);
  check('主题改成“深色”后界面立即变成深色', darkApplied === true);
  await page.evaluate(`window.ember.core.preferences.reset({ id: 'appearance.theme' })`);
  await page.evaluate(`window.ember.core.output.setVolume({ volume: 42 })`);
  const savedVolume = (await page.evaluate('window.ember.core.preferences.list({})')).settings.find((setting) => setting.id === 'sound.volume')?.value;
  check('音量保存到首选项（sound.volume）', savedVolume === 42, String(savedVolume));
  await page.evaluate(`window.ember.core.preferences.set({ id: 'sound.buffer', value: 'low' })`);
  const lowBuffer = (await page.evaluate('window.ember.core.preferences.list({})')).settings.find((setting) => setting.id === 'sound.buffer')?.value;
  check('缓冲大小可以改成“低延迟”并保存', lowBuffer === 'low', String(lowBuffer));
  await page.evaluate(`window.ember.core.preferences.reset({ id: 'sound.buffer' })`);
  await page.evaluate(`window.ember.core.preferences.reset({ id: 'sound.volume' })`);
  const afterReset = (await page.evaluate('window.ember.core.preferences.list({})')).settings;
  check('全部恢复默认后与出厂默认值一致（令牌除外）', afterReset.filter(comparable).every((setting) => JSON.stringify(setting.value) === JSON.stringify(setting.defaultValue)));

  // 调试服务：启动参数 --debug-port 强制开启，端点文件里有地址与令牌；用 WebSocket 握手、请求、订阅事件、发现接口；令牌不对被关闭；界面通道能读到服务状态。
  const endpoint = await waitFor(() => Promise.resolve(readEndpoint()), (value) => value !== undefined, 10_000, 100);
  check('调试服务按 --debug-port 开启，端点文件里有地址与令牌', endpoint?.port === debugPort && endpoint?.token === DEBUG_TOKEN && endpoint?.url === `ws://127.0.0.1:${debugPort}/`, JSON.stringify({ url: endpoint?.url, pid: endpoint?.pid }));
  const debugStatus = await page.evaluate('window.ember.core.debug.status({})');
  check('界面通道的 debug.status 显示已开启并在监听', debugStatus.enabled === true && debugStatus.listening === true && debugStatus.port === debugPort, JSON.stringify(debugStatus));
  const debugClient = await openDebugSocket(endpoint.url);
  const helloReply = await debugClient.call('debug.hello', { client: 'smoke', token: DEBUG_TOKEN });
  check('调试连接握手成功（协议 1，带应用版本）', helloReply.result?.protocol === 1 && typeof helloReply.result?.appVersion === 'string', JSON.stringify(helloReply.result));
  const viaDebug = (await debugClient.call('runtime.status')).result;
  const viaBridge = await page.evaluate('window.ember.core.runtime.status({})');
  check('经调试连接读到的运行状态与界面通道一致（电源、模式、卡带插件）', viaDebug?.power === viaBridge.power && viaDebug?.mode === viaBridge.mode && viaDebug?.cartridge?.pluginId === viaBridge.cartridge?.pluginId && viaDebug?.power === 'on', `${viaDebug?.power}/${viaDebug?.mode}`);
  const invalidParams = await debugClient.call('output.setVolume', { volume: 101 });
  check('调试连接上的参数校验与界面通道相同（JSON-RPC -32602）', invalidParams.error?.code === -32602 && invalidParams.error?.data?.code === 'core.invalidParams');
  const document = (await debugClient.call('rpc.discover')).result;
  const methodNames = new Set((document?.methods ?? []).map((method) => method.name));
  check('rpc.discover 返回 OpenRPC 文档，覆盖运行、卡带、首选项等请求', document?.openrpc === '1.3.2' && ['runtime.status', 'cartridges.load', 'preferences.set', 'debug.hello', 'rpc.discover'].every((name) => methodNames.has(name)), `${methodNames.size} 个方法`);
  const subscribed = await debugClient.call('debug.subscribe', { events: ['preferences.changed', 'debug.*'] });
  check('订阅事件成功', subscribed.result?.events?.length === 2);
  const padReply = await debugClient.call('input.pad', { port: 0, buttons: 8 });
  check('input.pad 设置手柄口成功，不存在的口返回 input.invalidPort', padReply.result !== undefined && (await debugClient.call('input.pad', { port: 7, buttons: 1 })).error?.data?.code === 'input.invalidPort');
  await debugClient.call('input.releaseAll');
  const ran = (await debugClient.call('runtime.run', { frames: 30, wait: true })).result;
  check('runtime.run 等待跑完 30 帧，停止原因 completed', ran?.stopped === 'completed' && ran?.frames === 30 && ran?.cycles > 0, JSON.stringify(ran));
  const shot = await debugClient.call('output.screenshot', { format: 'rgba' });
  check('output.screenshot 以二进制帧返回 256x240 的 RGBA 像素', shot.result?.width === 256 && shot.result?.height === 240 && shot.binary?.length === 256 * 240 * 4 && shot.result?.byteLength === shot.binary.length, `${shot.result?.byteLength}/${shot.binary?.length}`);
  const pngShot = (await debugClient.call('output.screenshot', { file: 'smoke-shot' })).result;
  check('output.screenshot 带 file 时保存 PNG 到调试输出目录', typeof pngShot?.file === 'string' && pngShot.file.endsWith('smoke-shot.png') && existsSync(pngShot.file) && pngShot.data === undefined, pngShot?.file);
  await debugClient.call('runtime.resume');
  await page.evaluate(`window.ember.core.preferences.set({ id: 'sound.startMuted', value: true })`);
  const pushed = await waitFor(() => Promise.resolve(debugClient.notifications.find((message) => message.method === 'preferences.changed')), (value) => value !== undefined, 5000, 50);
  check('在界面里改设置，调试连接收到 preferences.changed 通知', pushed?.params?.id === 'sound.startMuted' && pushed?.params?.value === true, JSON.stringify(pushed?.params));
  await page.evaluate(`window.ember.core.preferences.reset({ id: 'sound.startMuted' })`);
  const debugListed = (await debugClient.call('debug.status')).result;
  check('debug.status 里有这条连接', debugListed?.connections?.length === 1 && debugListed.connections[0].client === 'smoke', JSON.stringify(debugListed?.connections));
  const debugItem = () => page.evaluate(`(() => { const item = document.querySelector('.ui-status-bar__end .ui-status-bar__item[data-tone]'); return item && item.textContent === '调试模式' ? { tone: item.getAttribute('data-tone'), title: item.title } : null; })()`);
  const debugInfo = await waitFor(debugItem, (value) => value !== null && value.title.includes('1 个连接'), 5000, 100);
  check('系统栏右侧（通知按钮左边）显示绿色的“调试模式”，端口与连接数在悬停提示里', debugInfo?.tone === 'success' && debugInfo.title === `调试服务：ws://127.0.0.1:${debugPort}，1 个连接`, JSON.stringify(debugInfo));
  const wrongToken = await openDebugSocket(endpoint.url);
  const wrongReply = await wrongToken.call('debug.hello', { client: 'intruder', token: 'wrong' });
  await waitFor(() => Promise.resolve(wrongToken.closed), (closed) => closed === true, 3000, 50);
  check('令牌不对：返回 debug.unauthorized 并以 4001 关闭连接', wrongReply.error?.data?.code === 'debug.unauthorized' && wrongToken.closeCode === 4001, String(wrongToken.closeCode));
  const intruderRequest = await openDebugSocket(endpoint.url);
  const unauthorized = await intruderRequest.call('runtime.status');
  check('没有握手就发请求：被拒绝并关闭', unauthorized.error?.data?.code === 'debug.unauthorized');
  debugClient.close();

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
      const restartedButton = await waitFor(runButtonInfo, (info) => info?.title === '运行：未开机', 30_000, 200);
      check('核心重启后运行按钮重新读取（提示是“运行：未开机”，不再是旧的“运行：运行中”）', restartedButton?.title === '运行：未开机', JSON.stringify(restartedButton));
      if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'smoke-no-machine.png'));
      const restartedBar = await textOf('.ui-status-bar');
      check('核心重启后系统栏的卡带与 mapper 项同样刷新（不再显示旧的卡带名）', !restartedBar.includes('nestest.nes') && !restartedBar.includes('NROM'), restartedBar.replace(/\s+/g, ' ').slice(0, 60));
      // 调试服务在新核心里重新监听（启动参数会再次交给核心），令牌不变。
      const afterRestart = await waitFor(
        () => openDebugSocket(`ws://127.0.0.1:${debugPort}/`).catch(() => undefined),
        (value) => value !== undefined,
        20_000,
        300
      );
      const restartedHello = afterRestart === undefined ? undefined : await afterRestart.call('debug.hello', { client: 'smoke-2', token: DEBUG_TOKEN });
      check('核心进程重启后调试服务重新监听，原令牌仍然有效', restartedHello?.result?.protocol === 1, JSON.stringify(restartedHello?.result));
      afterRestart?.close();
    }
  }

  // 首选项对话框：核心重启之后（界面要重新读取）经“设置”菜单的“首选项”打开，是独立的系统窗口（第二个页面目标），左侧是分组，右侧是设置行。
  await menuButton('设置');
  await sleep(300);
  const settingsItems = await menuItems('.ui-menu__popup');
  check('“设置”菜单里有“首选项”（Ctrl+,）', settingsItems.some((entry) => entry.label === '首选项' && !entry.disabled && entry.text.includes('Ctrl+,')), settingsItems.map((entry) => entry.text).join('|'));
  await page.evaluate(`[...document.querySelectorAll('.ui-menu__popup [role^="menuitem"]')].find((item) => item.querySelector('.ui-menu__label')?.textContent === '首选项')?.click()`);
  const dialogTarget = await waitFor(
    async () => (await (await fetch(`http://127.0.0.1:${port}/json/list`)).json()).find((entry) => entry.type === 'page' && entry.id !== target.id),
    (value) => value !== undefined,
    10_000,
    200
  );
  check('首选项在独立的系统窗口（对话框）里打开', dialogTarget !== undefined, dialogTarget?.title);
  const dialogPage = dialogTarget === undefined ? undefined : await connectCdp(dialogTarget.webSocketDebuggerUrl);
  const dialogText = dialogPage === undefined ? '' : await waitFor(() => dialogPage.evaluate('document.body.innerText').catch(() => ''), (text) => text.includes('主题'), 5000, 100);
  check('首选项对话框：左侧有外观、画面、声音、调试服务四个分组，默认显示“外观”的“主题”', ['外观', '画面', '声音', '调试服务', '主题'].every((word) => dialogText.includes(word)), dialogText.replace(/\s+/g, ' ').slice(0, 80));
  const dialogTheme = dialogPage === undefined ? '' : await dialogPage.evaluate('document.documentElement.className');
  const mainTheme = await page.evaluate('document.documentElement.className');
  check('对话框的主题类与主窗口一致（深浅主题都有）', dialogTheme.split(' ').filter((name) => name.startsWith('ui-theme') || name.includes('dark') || name.includes('light')).join(' ') === mainTheme.split(' ').filter((name) => name.startsWith('ui-theme') || name.includes('dark') || name.includes('light')).join(' '), `${dialogTheme} / ${mainTheme}`);
  if (screenshotDirectory !== undefined && dialogPage !== undefined) await saveScreenshot(dialogPage, join(screenshotDirectory, 'smoke-preferences.png'));
  dialogPage?.close();

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
