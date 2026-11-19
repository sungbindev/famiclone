/*
 * ------------------------------------------------------------------------
 * 名称：capture-main.cjs
 * 说明：网页版截图程序（Electron 主进程脚本）：打开参考页，对一组“场景:样本序号”依次用真实的输入事件把样本带到指定状态，保存页面截图与各样本的位置。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：参数用 --key=value 形式（Electron 不接受不带 -- 前缀的 key=value，会直接静默退出）：
 *       page（参考页 html 路径）、name（示例页名）、theme、scale、width、height、cases（逗号分隔，如 normal,hover:0,focus:3）、prefix（输出文件的路径前缀）。
 *       每个场景输出 <prefix>-<场景>[<序号>].png 与同名 .json；场景与 gallery 的 --scene 一致：normal；hover（鼠标移到样本）；pressed（再按下左键）；focus（按 Tab 键直到焦点落在样本上）；
 *       open（在下拉框样本上点一下展开弹层）；openkey（Tab 到下拉框后按“向下”键展开）；openhover（先展开，再把鼠标移到弹层里的第三个选项）。
 *       样本是页面里带 data-sample 属性的元素，按文档顺序。
 *       缩放比例与页面大小用调试协议的设备指标覆盖指定，截图的像素大小严格等于 逻辑大小 × 缩放比例；
 *       同时用 force-device-scale-factor 让页面排版用的设备缩放比例也等于目标缩放比例：
 *       不这样做的话，边框、轮廓线的宽度（1px 取整到设备像素）会按本机系统的缩放比例算，截图的线宽因机器而异。
 *       窗口大小会被系统取整，所以截图尺寸只由设备指标覆盖决定。
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
app.commandLine.appendSwitch('force-device-scale-factor', String(scale));
const width = Number(options.width ?? '440');
const height = Number(options.height ?? '60');
// 悬停与颜色过渡的时间由页面决定（参考页已关闭过渡），这里只等一帧渲染
const SETTLE_MILLISECONDS = 200;
/** tip 场景：等悬停提示弹出的时间（提示延迟 400 毫秒）。 */
const TOOLTIP_MILLISECONDS = 700;
const KEY_TAB = 9;
const KEY_ARROW_DOWN = 40;
/** drag 场景：滑块向下拖的距离（与 gallery 的 kDragDistance 一致）。 */
const DRAG_DISTANCE = 30;
/** wheel 场景里发送的滚轮量（浏览器一格）。 */
const WHEEL_DELTA = 100;
/** 样本里滑块的中心。 */
const SLIDER_CENTER = `(() => { const r = document.querySelector('.ui-scroll-area .scrollbar.vertical .slider')?.getBoundingClientRect(); return r ? { x: r.x + r.width / 2, y: r.y + r.height / 2 } : null; })()`;
/** openhover 场景：弹层里第三个选项（下拉选项或菜单项）的中心（与 gallery 的 kHoverOption 一致）。 */
const HOVER_OPTION_CENTER = `(() => { const r = document.querySelectorAll('.ui-select__option, .ui-menu__item')[2]?.getBoundingClientRect(); return r ? { x: r.x + r.width / 2, y: r.y + r.height / 2 } : null; })()`;
/** 鼠标移到页面外时用的坐标。 */
const OUTSIDE = { x: -10, y: -10 };

/** 等待指定毫秒。 */
function delay(milliseconds) {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

/** 样本的外层盒子的类名：样本属性加在控件内部的元素上时（如输入框里的 input），位置取外层盒子的。 */
const BOX_SELECTOR = '.ui-input';

/** 样本元素：带 data-sample 属性的，以及组件库里不接收 data 属性的选择类控件（复选框、单选组、下拉框）；下拉框展开时弹层里的各个选项排在所有样本之后；面板里嵌的控件（滑块、面板自己的滚动区域）不算样本。 */
const SAMPLE_SELECTOR = '[data-sample], .ui-choice[role="checkbox"], .ui-choice-group, .ui-select, .ui-select__option, .ui-slider:not(:is(.ui-panel, .ui-field, .ui-setting-row, .ui-toolbar) .ui-slider), .ui-scroll-area:not(.ui-select__popup):not(.ui-panel__scroll):not(.ui-split-view__side-body):not(.ui-table-container):not(.ui-notification-center__scroll):not(.ui-dialog__scroll), .ui-panel, .ui-facts, .ui-field, .ui-setting-row:not(.ui-list .ui-setting-row), .ui-list, .ui-row-list__item, .ui-toolbar, .ui-tree, .ui-callout, .ui-state, .ui-status-label:not(.ui-table .ui-status-label), .ui-split-view, .ui-splitter, .ui-tabs, .ui-table, .ui-menubar, .ui-menu__popup, .ui-menu__item, .ui-title-bar, .ui-title-bar__actions > *, .ui-title-bar__tool, .ui-status-bar, .ui-status-bar__item, .ui-tooltip, .ui-notice-toast, .ui-notification-button, .ui-notification-center, .ui-notification-center__item, .ui-dialog-window__control, .ui-dialog__footer .ui-button, .ui-app-frame, .ui-app-frame__page, .ui-app-frame__bottom';

/** 取页面里全部样本的位置。 */
const SAMPLE_RECTS = `[...document.querySelectorAll('${SAMPLE_SELECTOR}')].map((element) => { const r = (element.closest("${BOX_SELECTOR}") || element).getBoundingClientRect(); return { x: r.x, y: r.y, width: r.width, height: r.height }; })`;

/** 目标样本之前（含）有多少个 Tab 停靠点：样本自己可获得焦点算一个；单选组、下拉框自己不能，里面当前可 Tab 到的元素算一个。 */
function tabCount(target) {
  // 样本里嵌着的另一个样本（如分栏视图里的分隔条）只在它自己那一项算一次
  return `(() => { const all = [...document.querySelectorAll('${SAMPLE_SELECTOR}')]; return all.slice(0, ${target + 1}).reduce((sum, element) => sum + (!element.disabled && element.tabIndex >= 0 ? 1 : 0) + [...element.querySelectorAll('[tabindex="0"], button:not(:disabled), input:not(:disabled)')].filter((inner) => !all.includes(inner)).length, 0); })()`;
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
  url.searchParams.set('page', options.name ?? 'button');
  const contents = window.webContents;
  await window.loadURL(url.href);
  contents.debugger.attach('1.3');
  const send = (method, params) => contents.debugger.sendCommand(method, params);
  await send('Emulation.setDeviceMetricsOverride', { width, height, deviceScaleFactor: scale, mobile: false });
  await send('Emulation.setFocusEmulationEnabled', { enabled: true });

  let mouse = OUTSIDE;
  let mouseDown = false;
  for (const item of (options.cases ?? 'normal').split(',')) {
    const [scene, targetText] = item.split(':');
    const target = Number(targetText ?? '0');
    // 回到干净状态：松开鼠标、移到页面外、重新载入页面
    if (mouseDown) await send('Input.dispatchMouseEvent', { type: 'mouseReleased', x: mouse.x, y: mouse.y, button: 'left', clickCount: 1 });
    mouseDown = false;
    await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: OUTSIDE.x, y: OUTSIDE.y, button: 'none' });
    mouse = OUTSIDE;
    await window.loadURL(url.href);

    const rects = await contents.executeJavaScript(SAMPLE_RECTS);
    const box = rects[target];
    if (scene !== 'normal' && !box) throw new Error(`样本 ${target} 不存在`);
    if (scene === 'hover' || scene === 'pressed') {
      mouse = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'none' });
    }
    if (scene === 'pressed') {
      await send('Input.dispatchMouseEvent', { type: 'mousePressed', x: mouse.x, y: mouse.y, button: 'left', clickCount: 1 });
      mouseDown = true;
    }
    if (scene === 'focus' || scene === 'openkey') {
      const presses = await contents.executeJavaScript(tabCount(target));
      for (let i = 0; i < presses; i += 1) {
        await send('Input.dispatchKeyEvent', { type: 'rawKeyDown', key: 'Tab', code: 'Tab', windowsVirtualKeyCode: KEY_TAB });
        await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Tab', code: 'Tab', windowsVirtualKeyCode: KEY_TAB });
      }
    }
    if (scene === 'openkey') {
      // 焦点在下拉框上时按“向下”键展开弹层（键盘展开，高亮落在已选项上）
      await send('Input.dispatchKeyEvent', { type: 'rawKeyDown', key: 'ArrowDown', code: 'ArrowDown', windowsVirtualKeyCode: KEY_ARROW_DOWN });
      await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'ArrowDown', code: 'ArrowDown', windowsVirtualKeyCode: KEY_ARROW_DOWN });
    }
    if (scene === 'tip') {
      // 鼠标移到样本中心，等到悬停提示弹出
      mouse = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'none' });
      await delay(TOOLTIP_MILLISECONDS);
    }
    if (scene === 'wheel') {
      // 先把鼠标移到样本中心，再向下滚一格
      mouse = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'none' });
      await send('Input.dispatchMouseEvent', { type: 'mouseWheel', x: mouse.x, y: mouse.y, deltaX: 0, deltaY: WHEEL_DELTA });
    }
    if (scene === 'drag') {
      // 鼠标移到样本上让滚动条显示，再在滑块上按下并向下拖
      mouse = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'none' });
      await delay(SETTLE_MILLISECONDS);
      const grab = await contents.executeJavaScript(SLIDER_CENTER);
      if (!grab) throw new Error('样本里没有滚动条');
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: grab.x, y: grab.y, button: 'none' });
      await send('Input.dispatchMouseEvent', { type: 'mousePressed', x: grab.x, y: grab.y, button: 'left', buttons: 1, clickCount: 1 });
      mouseDown = true;
      mouse = { x: grab.x, y: grab.y + DRAG_DISTANCE };
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'left', buttons: 1 });
    }
    if (scene === 'open' || scene === 'openhover') {
      // 在样本中心点一下展开弹层（鼠标展开，指针移到选项上才有高亮）
      mouse = { x: box.x + box.width / 2, y: box.y + box.height / 2 };
      await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'none' });
      await send('Input.dispatchMouseEvent', { type: 'mousePressed', x: mouse.x, y: mouse.y, button: 'left', clickCount: 1 });
      await send('Input.dispatchMouseEvent', { type: 'mouseReleased', x: mouse.x, y: mouse.y, button: 'left', clickCount: 1 });
      if (scene === 'openhover') {
        await delay(SETTLE_MILLISECONDS);
        const row = await contents.executeJavaScript(HOVER_OPTION_CENTER);
        // 禁用的下拉框展开不了，没有选项可悬停
        if (row) {
          mouse = row;
          await send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: mouse.x, y: mouse.y, button: 'none' });
        }
      }
    }
    await delay(SETTLE_MILLISECONDS);

    // 展开弹层之后才有选项元素，所以截图前重新取一次位置
    const finalRects = await contents.executeJavaScript(SAMPLE_RECTS);
    const shot = await send('Page.captureScreenshot', { format: 'png', fromSurface: true, clip: { x: 0, y: 0, width, height, scale: 1 } });
    const name = scene === 'normal' ? 'normal' : `${scene}${target}`;
    writeFileSync(`${options.prefix}-${name}.png`, Buffer.from(shot.data, 'base64'));
    writeFileSync(`${options.prefix}-${name}.json`, JSON.stringify(finalRects.map((rect, index) => ({ index, ...rect })), null, 2) + '\n');
  }
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
