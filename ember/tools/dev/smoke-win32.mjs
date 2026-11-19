/*
 * ------------------------------------------------------------------------
 * 名称：smoke-win32.mjs
 * 说明：原生外壳冒烟脚本：启动 ember-win.exe 加载一张测试卡带，用系统输入驱动窗口，检查画面、快捷键、菜单盖在画面上、菜单暂停与继续、运行面板展开与自动收缩、键盘独占、窗口始终置顶、全屏、正常关闭与退出；再起实例经菜单选学习机用随包的 BIOS 开机；再起一个带调试端口的实例，检查“调试模式”项、真实拖放软盘镜像载入软驱与设备面板的按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用法：node tools/dev/smoke-win32.mjs [--rom=相对 core/test/data 或绝对路径] [--no-build]；默认卡带是 apu-test/1-len_ctr.nes（只在画面左下角写字，中间是空的，便于判断提示与菜单）；
 *       判断方法与坐标见 win32-dev-support.mjs；每个场景都是一个独立的实例，只结束本脚本启动的进程（按 PID）；退出码 0 表示全部检查通过。
 *       快捷键的检查要在没有点过鼠标时做：点过之后焦点回到画面，键盘被独占，F8 这类外壳快捷键属于机器（见 docs/05 第 6 节）；全屏的实例退出不了全屏，只能关闭窗口，所以单独一个实例。
 * ------------------------------------------------------------------------
 */

import { spawnSync } from 'node:child_process';
import { writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { isAbsolute, join } from 'node:path';

import { debugCall, drive, dropFiles, parseArguments, projectRoot, removeUserData, sleep, startApp } from './win32-dev-support.mjs';

/** 默认卡带：1-len_ctr（blargg 的 APU 测试，通过后在画面左下角显示文字）。 */
const DEFAULT_ROM = 'apu-test/1-len_ctr.nes';
/** 关闭窗口后进程应在这个时间内退出（毫秒）。 */
const EXIT_LIMIT_MS = 6000;

/** 画面中央（暂停提示的图标与文字所在）的统计区域，逻辑像素。 */
const CENTER = [200, 190, 330, 290];
/** 画面左下角（卡带写字的地方）。 */
const TEXT_AREA = [10, 320, 250, 420];
/** “机器”菜单弹出的位置（盖在画面上）。 */
const MACHINE_MENU = [125, 45, 300, 190];
/** 展开的运行面板里按钮所在的一行。 */
const PANEL_ROW = [10, 425, 290, 462];
/** 显示区里画面所在的大块区域。 */
const PICTURE_AREA = [20, 40, 500, 440];
/** 系统栏右侧“调试模式”所在的位置。 */
const DEBUG_ITEM = [400, 472, 485, 498];
/** 展开的设备面板里软驱 A 一行的盘片名，以及行尾的写保护、取出按钮。 */
const FLOPPY_NAME = [120, 383, 235, 403];
const WRITE_PROTECT_BUTTON = 'click:437:393';
const EJECT_BUTTON = 'click:493:393';
/** 系统栏左侧的设备按钮。 */
const DEVICE_BUTTON = 'click:56:485';
/** 1.44 MB 软盘镜像的大小（字节）。 */
const FLOPPY_BYTES = 1_474_560;

const options = parseArguments(process.argv.slice(2));
const romOption = options.get('rom') ?? DEFAULT_ROM;
const romPath = isAbsolute(romOption) ? romOption : join(projectRoot, 'core', 'test', 'data', romOption);

const results = [];
/** 记录一项检查。 */
function check(name, ok, detail = '') {
  results.push({ name, ok });
  console.log(`${ok ? '通过' : '失败'}：${name}${detail ? `（${detail}）` : ''}`);
}

/** 取一个结果里的数字。 */
const number = (map, name) => Number((map.get(name) ?? 'NaN').split(' ')[0]);

/** 统计步骤。 */
const count = (name, area) => `count:${name}:${area.join(':')}`;

/** 等窗口出现并画出第一帧。 */
async function waitForWindow(pid) {
  for (let attempt = 0; attempt < 40; attempt += 1) {
    const result = drive(pid, [count('probe', TEXT_AREA)]);
    if (result.get('error') === undefined && number(result, 'probe') >= 0) return true;
    await sleep(500);
  }
  return false;
}

/** 正常关闭窗口并确认进程在时限内以 0 退出。 */
async function closeAndExpectExit(app, name) {
  drive(app.pid, ['close']);
  const code = await Promise.race([app.exited, sleep(EXIT_LIMIT_MS).then(() => 'timeout')]);
  check(`${name}：关闭窗口后进程正常退出`, code === 0, `退出码 ${code}`);
}

/** 场景一：快捷键、菜单、面板、键盘独占、置顶。 */
async function scenarioInteractions() {
  console.log('场景一：快捷键、菜单、面板、键盘独占与置顶');
  const app = startApp({ romPath });
  try {
    check('启动后出现窗口', await waitForWindow(app.pid));
    await sleep(3000);

    // 没有点过鼠标：焦点不在画面上，外壳快捷键有效
    let r = drive(app.pid, [count('textRunning', TEXT_AREA), count('centerRunning', CENTER)]);
    check('卡带的画面显示出来', number(r, 'textRunning') > 100, `左下角亮像素 ${number(r, 'textRunning')}`);
    const centerRunning = number(r, 'centerRunning');
    check('没有开启调试服务时系统栏没有“调试模式”', number(drive(app.pid, [count('noDebug', DEBUG_ITEM)]), 'noDebug') === 0);
    r = drive(app.pid, ['key:F8', 'wait:600', count('centerPaused', CENTER)]);
    check('F8 暂停并显示“已暂停”提示', number(r, 'centerPaused') > centerRunning + 40, `${centerRunning} → ${number(r, 'centerPaused')}`);

    // 菜单盖在画面上（暂停期间的画面上也一样）
    r = drive(app.pid, ['click:151:19', 'wait:700', count('menuOpen', MACHINE_MENU)]);
    check('“机器”菜单盖在画面上显示出来', number(r, 'menuOpen') > 200, `亮像素 ${number(r, 'menuOpen')}`);
    r = drive(app.pid, ['key:ESC', 'wait:500', count('menuClosed', MACHINE_MENU)]);
    check('Esc 关闭菜单，画面露出来', number(r, 'menuClosed') < 20, `亮像素 ${number(r, 'menuClosed')}`);

    // 菜单项：继续与暂停（F8 之后焦点回到画面、键盘被独占，所以后面的操作都用鼠标）
    r = drive(app.pid, ['click:151:19', 'wait:500', 'click:165:73', 'wait:700', count('menuResumed', CENTER)]);
    check('菜单里选“继续”后撤掉提示', number(r, 'menuResumed') <= centerRunning + 10, `${number(r, 'menuResumed')}`);
    r = drive(app.pid, ['click:151:19', 'wait:500', 'click:165:73', 'wait:700', count('menuPaused', CENTER)]);
    check('菜单里选“暂停”后显示提示', number(r, 'menuPaused') > centerRunning + 40, `${number(r, 'menuPaused')}`);
    drive(app.pid, ['click:151:19', 'wait:500', 'click:165:73', 'wait:700']);

    // 运行面板：点系统栏左侧的运行按钮展开，点画面收起；点过之后焦点回到画面，键盘被独占
    r = drive(app.pid, [count('panelClosed', PANEL_ROW), 'click:24:485', 'wait:800', count('panelOpen', PANEL_ROW)]);
    const panelClosed = number(r, 'panelClosed');
    check('点系统栏的运行按钮展开运行面板', number(r, 'panelOpen') > panelClosed + 50, `${panelClosed} → ${number(r, 'panelOpen')}`);
    r = drive(app.pid, ['click:260:150', 'wait:800', count('panelCollapsed', PANEL_ROW)]);
    check('点画面后运行面板自动收起', number(r, 'panelCollapsed') <= panelClosed + 20, `${number(r, 'panelCollapsed')}`);
    r = drive(app.pid, ['key:F8', 'wait:600', count('exclusive', CENTER)]);
    check('画面持有焦点时键盘被独占，F8 不触发外壳的暂停', number(r, 'exclusive') <= centerRunning + 10, `${number(r, 'exclusive')}`);

    // 窗口始终置顶：勾选项切换后菜单保持打开，所以连点两次菜单项再按 Esc 关闭
    r = drive(app.pid, ['style:before', 'click:211:19', 'wait:500', 'click:253:49', 'wait:700', 'style:on']);
    check('“窗口始终置顶”勾选后窗口置顶', (r.get('before') ?? '').startsWith('False') && (r.get('on') ?? '').startsWith('True'), `${r.get('before')} → ${r.get('on')}`);
    r = drive(app.pid, ['click:253:49', 'wait:700', 'style:off', 'key:ESC', 'wait:400']);
    check('再点一次取消置顶', (r.get('off') ?? '').startsWith('False'), `${r.get('off')}`);

    await closeAndExpectExit(app, '场景一');
  } finally {
    app.stop();
    removeUserData(app);
  }
}

/** 场景二：全屏（F11 要在没点过鼠标的实例里按）。 */
async function scenarioFullScreen() {
  console.log('场景二：全屏');
  const app = startApp({ romPath });
  try {
    check('启动后出现窗口', await waitForWindow(app.pid));
    await sleep(2500);
    const r = drive(app.pid, ['style:normal', 'key:F11', 'wait:1500', 'style:full']);
    const [, normalWidth] = (r.get('normal') ?? '').split(' ').map(Number);
    const [, fullWidth] = (r.get('full') ?? '').split(' ').map(Number);
    check('F11 进入全屏，窗口比原来大', fullWidth > normalWidth * 1.5, `宽 ${normalWidth} → ${fullWidth}`);
    await closeAndExpectExit(app, '场景二');
  } finally {
    app.stop();
    removeUserData(app);
  }
}

/** 场景三：经“文件 → 选择机型”选小霸王 2000，用随包的 BIOS 开机，画面是满屏的 BIOS 硬件检测页。 */
async function scenarioBios() {
  console.log('场景三：学习机 BIOS 开机');
  const app = startApp({});
  try {
    check('启动后出现窗口', await waitForWindow(app.pid));
    await sleep(2000);
    const r = drive(app.pid, ['click:92:19', 'wait:600', 'click:120:49', 'wait:800', 'click:330:78', 'wait:12000', count('biosPicture', PICTURE_AREA)]);
    check('选择小霸王 2000 后 BIOS 开机，画面铺满显示区', number(r, 'biosPicture') > 20000, `亮像素 ${number(r, 'biosPicture')}`);
    await closeAndExpectExit(app, '场景三');
  } finally {
    app.stop();
    removeUserData(app);
  }
}

/**
 * 场景四：调试服务与设备面板。带 --debug-port 启动，系统栏出现“调试模式”；用调试 SDK 的终端客户端让学习机开机，把软盘镜像真实拖进窗口载入软驱，
 * 展开设备面板，点写保护与取出按钮，再用终端客户端核对软驱状态。
 */
async function scenarioDevices() {
  console.log('场景四：调试服务与设备面板');
  const app = startApp({ args: ['--debug-port=5798', '--debug-token=smoke-token'] });
  try {
    check('启动后出现窗口', await waitForWindow(app.pid));
    await sleep(2500);
    check('开启调试服务后系统栏出现“调试模式”', number(drive(app.pid, [count('debugItem', DEBUG_ITEM)]), 'debugItem') > 20);
    const status = debugCall(app, 'debug.status');
    check('调试服务在监听', status.enabled === true && status.listening === true && status.port === 5798, JSON.stringify(status));
    const boot = debugCall(app, 'machines.boot', 'machine=sb2000');
    check('经调试服务让小霸王 2000 开机', boot.state.power === 'on');

    const image = join(tmpdir(), `ember-smoke-${process.pid}.img`);
    writeFileSync(image, Buffer.alloc(FLOPPY_BYTES));
    check('拖放完成', dropFiles(app.pid, [image], { x: 262, y: 200 }));
    await sleep(1000);
    const inserted = debugCall(app, 'media.floppyStatus').drives[0];
    check('把软盘镜像拖进窗口后载入了软驱 A', inserted?.inserted === true && inserted.fileName.endsWith('.img'), inserted?.fileName);

    let r = drive(app.pid, [DEVICE_BUTTON, 'wait:900', count('floppyName', FLOPPY_NAME)]);
    check('点设备按钮展开设备面板，软驱 A 一行显示盘片名', number(r, 'floppyName') > 50, `亮像素 ${number(r, 'floppyName')}`);
    drive(app.pid, [WRITE_PROTECT_BUTTON, 'wait:800']);
    check('点写保护按钮后盘片写保护', debugCall(app, 'media.floppyStatus').drives[0].writeProtected === true);
    drive(app.pid, [EJECT_BUTTON, 'wait:800']);
    check('点取出按钮后软驱里没有盘片', debugCall(app, 'media.floppyStatus').drives[0].inserted === false);
    r = drive(app.pid, [count('afterEject', FLOPPY_NAME)]);
    check('取出后盘片名换成“空”（盘片名那一列的亮像素从 500 以上降到 200 以下）', number(r, 'afterEject') < 200, `亮像素 ${number(r, 'afterEject')}`);
    await closeAndExpectExit(app, '场景四');
  } finally {
    app.stop();
    removeUserData(app);
  }
}

if (!options.has('no-build')) {
  const run = spawnSync('npm.cmd', ['run', 'win32:build'], { cwd: projectRoot, stdio: 'inherit', shell: true });
  if (run.status !== 0) throw new Error('npm run win32:build 失败');
}
await scenarioInteractions();
await scenarioFullScreen();
await scenarioBios();
await scenarioDevices();
const failed = results.filter((item) => !item.ok);
console.log(failed.length === 0 ? `全部 ${results.length} 项通过` : `${failed.length} 项失败：${failed.map((item) => item.name).join('；')}`);
process.exit(failed.length === 0 ? 0 : 1);
