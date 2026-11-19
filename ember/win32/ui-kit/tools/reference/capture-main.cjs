/*
 * ------------------------------------------------------------------------
 * 名称：capture-main.cjs
 * 说明：网页版截图程序（Electron 主进程脚本）：打开参考页，用真实的输入事件把按钮带到指定状态，保存页面截图与各按钮的位置。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：参数用 --key=value 形式（Electron 不接受不带 -- 前缀的 key=value，会直接静默退出）：
 *       page（参考页 html 路径）、theme、scale、scene、width、height、out（PNG）、rects（JSON）。
 *       场景与 gallery 的 --scene 一致：normal；hover（鼠标移到第一个按钮）；pressed（再按下左键）；focus（按 Tab 键）。
 *       缩放比例与页面大小用调试协议的设备指标覆盖指定，截图的像素大小严格等于 逻辑大小 × 缩放比例；
 *       不用窗口大小与 force-device-scale-factor，因为窗口尺寸会被系统取整，截图会被重采样而出现边缘振铃。
 * ------------------------------------------------------------------------
 */

'use strict';

const { app, BrowserWindow } = require('electron');
const { writeFileSync } = require('node:fs');
const { pathToFileURL } = require('node:url');

/** 读取 --key=value 参数。 */
function readArguments() {
  const values = {};
  for (const argument of process.argv.slice(2)) {
    const match = /^--([a-z]+)=(.*)$/.exec(argument);
    if (match) values[match[1]] = match[2];
  }
  return values;
}

const options = readArguments();
const scale = Number(options.scale ?? '1');
const width = Number(options.width ?? '440');
const height = Number(options.height ?? '60');
// 悬停与颜色过渡的时间由页面决定（参考页已关闭过渡），这里只等一帧渲染
const SETTLE_MILLISECONDS = 300;
const KEY_TAB = 9;

/** 等待指定毫秒。 */
function delay(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

async function capture() {
  const window = new BrowserWindow({
    width,
    height,
    show: false,
    frame: false,
    skipTaskbar: true,
    focusable: false,
    backgroundColor: '#000000',
    webPreferences: { sandbox: true, contextIsolation: true, nodeIntegration: false, backgroundThrottling: false }
  });
  // 隐藏的窗口不出画面，截图会一直等；改为完全透明、不抢焦点、不挡鼠标地“显示”出来
  window.setOpacity(0);
  window.setIgnoreMouseEvents(true);
  window.showInactive();
  const url = new URL(pathToFileURL(options.page).href);
  url.searchParams.set('theme', options.theme ?? 'dark');
  const contents = window.webContents;
  await window.loadURL(url.href);
  contents.debugger.attach('1.3');
  const send = (method, params) => contents.debugger.sendCommand(method, params);
  await send('Emulation.setDeviceMetricsOverride', { width, height, deviceScaleFactor: scale, mobile: false });
  await send('Emulation.setFocusEmulationEnabled', { enabled: true });

  const rects = await contents.executeJavaScript('[...document.querySelectorAll("button")].map((button) => { const r = button.getBoundingClientRect(); return { x: r.x, y: r.y, width: r.width, height: r.height }; })');
  const first = rects[0];
  const center = { x: first.x + first.width / 2, y: first.y + first.height / 2 };
  const scene = options.scene ?? 'normal';
  if (scene === 'hover' || scene === 'pressed') {
    await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: center.x, y: center.y, button: 'none' });
  }
  if (scene === 'pressed') {
    await send('Input.dispatchMouseEvent', { type: 'mousePressed', x: center.x, y: center.y, button: 'left', clickCount: 1 });
  }
  if (scene === 'focus') {
    await send('Input.dispatchKeyEvent', { type: 'rawKeyDown', key: 'Tab', code: 'Tab', windowsVirtualKeyCode: KEY_TAB });
    await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Tab', code: 'Tab', windowsVirtualKeyCode: KEY_TAB });
  }
  await delay(SETTLE_MILLISECONDS);

  const shot = await send('Page.captureScreenshot', { format: 'png', fromSurface: true, clip: { x: 0, y: 0, width, height, scale: 1 } });
  writeFileSync(options.out, Buffer.from(shot.data, 'base64'));
  writeFileSync(options.rects, JSON.stringify(rects.map((rect, index) => ({ index, ...rect })), null, 2) + '\n');
}

app.whenReady().then(async () => {
  try {
    await capture();
    app.exit(0);
  } catch (error) {
    process.stderr.write(`${error instanceof Error ? error.stack : String(error)}\n`);
    app.exit(1);
  }
});
