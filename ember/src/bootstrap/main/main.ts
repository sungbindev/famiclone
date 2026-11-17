/*
 * ------------------------------------------------------------------------
 * 名称：main.ts
 * 说明：主进程组装根：Windows 安装事件、单实例、错误日志、启动核心进程、登记核心请求与主进程请求的 IPC、画面与输入端口、外壳能力（界面状态的保存、窗口动作、键盘独占）、应用升级（Squirrel 引擎、启动时自动检查、状态推送）、创建主窗口，退出时保存界面状态并停止核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只创建实现并注入，不写业务判断；原生模块与预装插件在开发时取仓库根目录下的 build/ember/core/Release 与 build/ember/plugins，正式包取应用的 resources 目录；
 *       命令行带 --cartridge=路径（.nes）时，核心启动后由主进程加载这张卡带；界面状态保存在用户数据目录的 config/shell-state.json。
 * ------------------------------------------------------------------------
 */

import { dirname, join } from 'node:path';
import { existsSync } from 'node:fs';
import { app, BrowserWindow, ipcMain, shell } from 'electron';
import handledSquirrelEvent from 'electron-squirrel-startup';
import { CORE_SCHEMAS } from '../../contracts/core-gateway/generated/core-api';
import { STREAM_PORTS_WINDOW_MESSAGE } from '../../contracts/stream-ports/stream-messages';
import { STREAM_CHANNELS } from '../../contracts/stream-ports/stream-ports';
import { forwardCoreToWindow, registerCoreIpc, type CoreRequestHandling } from '../../entry/ipc/core/core-ipc';
import { loadAndRemember } from '../../entry/ipc/cartridges/load-cartridge';
import { registerMainRequestIpc, type MainRequestHandler } from '../../entry/ipc/core/main-request-ipc';
import { forwardKeyboardToWindow, forwardWindowState, registerShellIpc } from '../../entry/ipc/shell/shell-ipc';
import { forwardUpdateStatus } from '../../entry/ipc/system/forward-update-status';
import { registerStreamsIpc } from '../../entry/ipc/streams/streams-ipc';
import { startThemeSync } from '../../entry/preferences/theme-sync';
import { startAutoUpdateCheck, type AutoUpdateCheck } from '../../entry/updates/auto-update-check';
import { showReleaseNotesOnce } from '../../entry/updates/release-notes-prompt';
import { createSenderCheck } from '../../entry/ipc/sender-check';
import { ProcessCoreGateway } from '../../infra/core-client/process-core-gateway';
import { MemoryDroppedFilesService } from '../../infra/dropped-files/memory-dropped-files-service';
import { FileLastDirectoriesService } from '../../infra/last-directories/file-last-directories-service';
import { FileRecentCartridgesService } from '../../infra/recent-cartridges/file-recent-cartridges-service';
import { FileShellStateService } from '../../infra/shell-state/file-shell-state-service';
import { StreamPortHub } from '../../infra/stream-ports/stream-port-hub';
import { EngineAppUpdater } from '../../infra/updates/engine-app-updater';
import { startElectronCoreProcess } from '../../platform/core-process/core-process';
import { createOpenFileDialog } from '../../platform/dialog/open-file-dialog';
import { registerHelpProtocol, registerHelpScheme } from '../../platform/help/help-protocol';
import { createHelpWindow } from '../../platform/help/help-window';
import { cartridgePathFromArgv, debugOptionsFromArgv } from '../../platform/launch/launch-args';
import { loadNativeKeyboard, NativeKeyboardCapture } from '../../platform/keyboard-capture/native-keyboard-capture';
import { FileErrorLogger } from '../../platform/logging/file-error-logger';
import { createDirectoryOpener } from '../../platform/logging/log-directory';
import { JsonFileStore } from '../../platform/shell-state/json-file-store';
import { createElectronChannelFactory, createWebContentsStreamTarget } from '../../platform/stream-ports/electron-stream-ports';
import { findSquirrelLauncher } from '../../platform/updates/squirrel-install';
import { createSquirrelUpdateEngine } from '../../platform/updates/squirrel-update-engine';
import { applyThemeSource, createAppWindow, followSystemTheme, isAppUrl, removeDefaultMenu, restrictPermissions } from '../../platform/window/app-window';
import { createWindowController } from '../../platform/window/window-controller';
import { watchWindow } from '../../platform/window/window-events';
import { MAIN_MODULES } from './module-list';

/** 应用标识，与 forge.config.mts 一致。 */
const APP_ID = 'cn.3578.ember';
/** 产品名。 */
const PRODUCT_NAME = 'Ember';
/** 原生模块文件名。 */
const NATIVE_FILE = 'ember_core.node';
/** 键盘独占模块（只有 Windows 有）的文件名。 */
const KEYBOARD_NATIVE_FILE = 'ember_keyboard.node';
/** 预装插件的目录名：开发时在 build/ember 下，正式包在 resources 下。 */
const PLUGINS_DIRECTORY = 'plugins';
/** 默认 BIOS 的目录名（<机型>/<文件>，纯 BIOS 文件）：开发时在项目目录下，正式包在 resources 下。 */
const BIOS_DIRECTORY = 'bios';
/** 帮助文档站点的目录名：开发时在 build/ember 下，正式包在 resources 下。 */
const HELP_DIRECTORY = 'help';
/** 平台与架构的名字（如 win32-x64），ffmpeg 程序放在以它命名的目录里：开发时在 core/third_party/ffmpeg/bin 下，正式包在 resources 下。 */
const PLATFORM_DIRECTORY = `${process.platform}-${process.arch}`;
/** ffmpeg 程序的文件名。 */
const FFMPEG_FILE = process.platform === 'win32' ? 'ffmpeg.exe' : 'ffmpeg';
/** 主进程错误的错误码。 */
const MAIN_ERROR_CODE = 'shell.mainError';
/** 显示窗口前最多等保存的主题多久（毫秒），核心起不来时不能一直不显示。 */
const THEME_WAIT_MS = 2000;
/** 升级地址：存放 Squirrel 的 RELEASES 与 .nupkg 的静态目录（阿里云 OSS 的 famiclone Bucket），只写在这里，界面不能指定。 */
const UPDATE_FEED_URL = 'https://famiclone.oss-cn-beijing.aliyuncs.com/updates/ember/windows';
/** 启动后等多久再自动检查更新（毫秒），避开启动时的资源争抢。 */
const AUTO_UPDATE_CHECK_DELAY_MS = 10_000;
/** Squirrel 安装后第一次启动带的参数：这几秒 Squirrel 锁着应用文件，升级请求会失败，所以不自动检查。 */
const SQUIRREL_FIRST_RUN_ARGUMENT = '--squirrel-firstrun';

/** 把异常写入日志。 */
function logError(logger: FileErrorLogger, error: unknown, context: Record<string, unknown>): void {
  logger.write({
    process: 'main',
    code: MAIN_ERROR_CODE,
    message: error instanceof Error ? error.message : String(error),
    ...(error instanceof Error && error.stack ? { stack: error.stack } : {}),
    context
  });
}

// 安装、更新、卸载时 Squirrel 会带参数启动应用，处理快捷方式后立即退出。
// 对话框盖住整个主窗口时，Windows 上 Chromium 会把主窗口当作不可见而停掉渲染，主题等经主窗口同步给对话框的状态就停住；关掉遮挡检测让主窗口始终渲染。
if (process.platform === 'win32') app.commandLine.appendSwitch('disable-features', 'CalculateNativeWinOcclusion');
if (handledSquirrelEvent || !app.requestSingleInstanceLock()) {
  app.quit();
} else {
  if (process.platform === 'win32') app.setAppUserModelId(APP_ID);
  // 帮助文档的自定义协议要在应用就绪之前声明特权。
  registerHelpScheme();

  const userDataPath = app.getPath('userData');
  const logger = new FileErrorLogger(join(userDataPath, 'logs'));
  process.on('uncaughtException', (error) => logError(logger, error, { source: 'uncaughtException' }));
  process.on('unhandledRejection', (reason) => logError(logger, reason, { source: 'unhandledRejection' }));

  const devServerUrl = MAIN_WINDOW_VITE_DEV_SERVER_URL;
  const rendererFile = join(__dirname, `../renderer/${MAIN_WINDOW_VITE_NAME}/index.html`);
  const buildRoot = join(app.getAppPath(), '..', 'build', 'ember');
  const nativePath = app.isPackaged ? join(process.resourcesPath, NATIVE_FILE) : join(buildRoot, 'core', 'Release', NATIVE_FILE);
  const keyboardNativePath = app.isPackaged ? join(process.resourcesPath, KEYBOARD_NATIVE_FILE) : join(buildRoot, 'core', 'Release', KEYBOARD_NATIVE_FILE);
  const bundledPluginsPath = app.isPackaged ? join(process.resourcesPath, PLUGINS_DIRECTORY) : join(buildRoot, PLUGINS_DIRECTORY);
  const bundledBiosPath = app.isPackaged ? join(process.resourcesPath, BIOS_DIRECTORY) : join(app.getAppPath(), BIOS_DIRECTORY);
  const helpRoot = app.isPackaged ? join(process.resourcesPath, HELP_DIRECTORY) : join(buildRoot, HELP_DIRECTORY);
  const ffmpegPath = app.isPackaged
    ? join(process.resourcesPath, PLATFORM_DIRECTORY, FFMPEG_FILE)
    : join(app.getAppPath(), 'core', 'third_party', 'ffmpeg', 'bin', PLATFORM_DIRECTORY, FFMPEG_FILE);
  // 开发版用 resources/icon.ico 做窗口图标（打包后窗口用可执行文件里的图标；macOS 的 Dock 图标以后再定）。
  const windowIcon = app.isPackaged || process.platform === 'darwin' ? undefined : join(app.getAppPath(), 'resources', 'icon.ico');
  const helpWindow = createHelpWindow({ root: helpRoot, ...(windowIcon ? { icon: windowIcon } : {}) });
  // --debug-port 指定时本次运行强制开启调试服务（不写入首选项），--debug-token 指定令牌。
  const debugLaunch = debugOptionsFromArgv(process.argv);

  const gateway = new ProcessCoreGateway({
    startProcess: () => startElectronCoreProcess(join(__dirname, 'core-host.cjs')),
    nativePath,
    userDataPath,
    appVersion: app.getVersion(),
    bundledPluginsPath,
    bundledBiosPath,
    ffmpegPath,
    ...(debugLaunch.port === undefined ? {} : { debugPort: debugLaunch.port }),
    ...(debugLaunch.token === undefined ? {} : { debugToken: debugLaunch.token }),
    logger
  });
  const streamHub = new StreamPortHub({ channels: createElectronChannelFactory(), core: gateway, logger });
  const shellState = new FileShellStateService(
    new JsonFileStore({ path: join(userDataPath, 'config', 'shell-state.json'), onError: (error) => logError(logger, error, { source: 'shellState' }) })
  );
  const recentCartridges = new FileRecentCartridgesService(
    new JsonFileStore({ path: join(userDataPath, 'config', 'recent-cartridges.json'), onError: (error) => logError(logger, error, { source: 'recentCartridges' }) })
  );
  const droppedFiles = new MemoryDroppedFilesService();
  const lastDirectories = new FileLastDirectoriesService(
    new JsonFileStore({ path: join(userDataPath, 'config', 'last-directories.json'), onError: (error) => logError(logger, error, { source: 'lastDirectories' }) })
  );
  // 只有 Squirrel 安装的正式版支持升级，开发版、免安装目录和其他系统的状态是 unsupported。
  const squirrelLauncher = findSquirrelLauncher({ platform: process.platform, isPackaged: app.isPackaged, execPath: process.execPath, exists: existsSync });
  const updater = new EngineAppUpdater({
    engine: createSquirrelUpdateEngine({
      feedUrl: UPDATE_FEED_URL,
      launcher: squirrelLauncher,
      // Squirrel 的报错是系统代码页的文字，界面只给固定说明，原始报错与它自己的日志位置写进错误日志。
      onDiagnostic: (error) =>
        logError(logger, error, { source: 'updateEngine', ...(squirrelLauncher === undefined ? {} : { squirrelLog: join(dirname(squirrelLauncher), 'Squirrel-CheckForUpdate.log') }) })
    })
  });
  updater.onStatusChange((status) => {
    if (status.state === 'error') logError(logger, new Error(status.message ?? '升级失败'), { source: 'update' });
  });
  let autoUpdateCheck: AutoUpdateCheck | undefined;

  let mainWindow: BrowserWindow | undefined;
  let quitting = false;
  // 首选项里的主题由核心保存，读到后设为系统的主题来源；显示窗口前等它（最多 2 秒），避免先闪一下错误的颜色。
  const themeSync = startThemeSync({ gateway, applyThemeSource, onError: (error) => logError(logger, error, { source: 'themeSync' }) });
  const themeReady = Promise.race([themeSync.first, new Promise<void>((resolve) => setTimeout(resolve, THEME_WAIT_MS))]);
  const windowController = createWindowController(() => mainWindow, () => app.quit());
  const keyboard = new NativeKeyboardCapture({
    load: () => loadNativeKeyboard(keyboardNativePath, process.platform, (message) => logError(logger, new Error(message), { source: 'keyboardCapture' })),
    getWindow: () => mainWindow,
    onError: (message) => logError(logger, new Error(message), { source: 'keyboardCapture' })
  });
  const isTrustedSender = createSenderCheck((url) => isAppUrl(url, { rendererFile, ...(devServerUrl ? { devServerUrl } : {}) }));
  const modules = MAIN_MODULES.map((module) => module.schema);
  /** 以主进程的身份向核心发起请求所需的依赖。 */
  const coreHandling: CoreRequestHandling = { gateway, modules, schemas: CORE_SCHEMAS };

  const createMainWindow = (): void => {
    const window = createAppWindow({
      preloadPath: join(__dirname, 'preload.cjs'),
      rendererFile,
      title: PRODUCT_NAME,
      ...(windowIcon ? { icon: windowIcon } : {}),
      devTools: !app.isPackaged,
      placement: shellState.get().window,
      showAfter: themeReady,
      ...(devServerUrl ? { devServerUrl } : {})
    });
    mainWindow = window;
    const stopForwarding = forwardCoreToWindow(gateway, window.webContents);
    const stopKeyboardForwarding = forwardKeyboardToWindow(keyboard, window.webContents);
    const stopFollowingTheme = followSystemTheme(window);
    const stopUpdateForwarding = forwardUpdateStatus(updater, window.webContents);
    watchWindow(window, {
      onPlacement: (placement) => shellState.updateWindow(placement),
      onState: forwardWindowState(window.webContents, () => windowController.state())
    });
    window.webContents.on('render-process-gone', (_event, details) => logError(logger, new Error(`渲染进程退出：${details.reason}`), { exitCode: details.exitCode }));
    // 页面重新加载（开发时刷新）后界面的状态丢失，独占必须随之关闭，否则页面没有焦点时也吞掉所有按键。
    window.webContents.on('did-start-loading', () => keyboard.setCapture(false));
    window.on('closed', () => {
      stopForwarding();
      stopKeyboardForwarding();
      stopFollowingTheme();
      stopUpdateForwarding();
      // 帮助文档窗口随主窗口一起关闭，否则它一个窗口留着时应用退出不了。
      helpWindow.close();
      if (mainWindow === window) mainWindow = undefined;
    });
  };

  app.on('second-instance', () => {
    if (mainWindow === undefined) return;
    if (mainWindow.isMinimized()) mainWindow.restore();
    mainWindow.focus();
  });

  app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
  });

  app.on('activate', () => {
    if (app.isReady() && BrowserWindow.getAllWindows().length === 0) createMainWindow();
  });

  // 全部窗口关闭后再停止核心。
  app.on('will-quit', (event) => {
    if (quitting) return;
    quitting = true;
    event.preventDefault();
    shellState.flush();
    recentCartridges.flush();
    lastDirectories.flush();
    keyboard.dispose();
    themeSync.stop();
    autoUpdateCheck?.stop();
    streamHub.stop();
    void gateway
      .stop()
      .catch((error: unknown) => logError(logger, error, { source: 'stopCore' }))
      .then(() => logger.flush())
      .finally(() => app.quit());
  });

  void app.whenReady().then(() => {
    restrictPermissions({ rendererFile, ...(devServerUrl ? { devServerUrl } : {}) });
    registerHelpProtocol(helpRoot);
    removeDefaultMenu();
    streamHub.start();
    gateway.start();
    if (!process.argv.includes(SQUIRREL_FIRST_RUN_ARGUMENT)) {
      autoUpdateCheck = startAutoUpdateCheck({ gateway, updater, delayMs: AUTO_UPDATE_CHECK_DELAY_MS, onError: (error) => logError(logger, error, { source: 'autoUpdateCheck' }) });
    }
    registerCoreIpc({ ipc: ipcMain, gateway, modules, schemas: CORE_SCHEMAS, isTrustedSender });
    registerShellIpc({ ipc: ipcMain, isTrustedSender, state: shellState, window: windowController, keyboard, droppedFiles });

    const fileDialog = createOpenFileDialog(() => mainWindow, lastDirectories);
    const logDirectory = createDirectoryOpener(join(userDataPath, 'logs'), (target) => shell.openPath(target));
    const biosDirectory = createDirectoryOpener(join(userDataPath, 'bios'), (target) => shell.openPath(target));
    const handlers: Record<string, MainRequestHandler> = {};
    for (const module of MAIN_MODULES) Object.assign(handlers, module.createMainHandlers?.({ fileDialog, droppedFiles, logDirectory, biosDirectory, helpWindow, core: coreHandling, recentCartridges, updater }));
    registerMainRequestIpc({ ipc: ipcMain, handlers, modules, schemas: CORE_SCHEMAS, isTrustedSender, onError: (error, name) => logError(logger, error, { source: 'mainRequest', name }) });
    registerStreamsIpc({
      ipc: ipcMain,
      connector: streamHub,
      isTrustedSender,
      isStreamWindow: (contents) => mainWindow?.webContents === contents,
      createTarget: (contents) => createWebContentsStreamTarget(contents, STREAM_CHANNELS.ports, STREAM_PORTS_WINDOW_MESSAGE)
    });
    createMainWindow();
    // 更新记录页在主窗口显示之后再打开，否则会被随后显示的主窗口盖住。
    const promptReleaseNotes = (): void =>
      showReleaseNotesOnce({
        updater,
        currentVersion: app.getVersion(),
        firstRun: process.argv.includes(SQUIRREL_FIRST_RUN_ARGUMENT),
        state: shellState,
        helpWindow,
        onError: (error) => logError(logger, error, { source: 'releaseNotes' })
      });
    if (mainWindow?.isVisible() === true) promptReleaseNotes();
    else mainWindow?.once('show', promptReleaseNotes);

    const cartridgePath = cartridgePathFromArgv(process.argv);
    if (cartridgePath !== undefined) {
      loadAndRemember({ core: coreHandling, recent: recentCartridges }, cartridgePath).catch((error: unknown) => logError(logger, error, { source: 'launchCartridge' }));
    }
  });
}
