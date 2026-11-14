/*
 * ------------------------------------------------------------------------
 * 名称：main.ts
 * 说明：主进程组装根：Windows 安装事件、单实例、错误日志、启动核心进程、登记核心请求与主进程请求的 IPC、画面与输入端口、外壳能力（界面状态的保存、窗口动作）、创建主窗口，退出时保存界面状态并停止核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只创建实现并注入，不写业务判断；原生模块与预装插件在开发时取 core/build/Release 与 build/plugins，正式包取应用的 resources 目录；
 *       命令行带 --cartridge=路径（.nes）时，核心启动后由主进程加载这张卡带；界面状态保存在用户数据目录的 config/shell-state.json。
 * ------------------------------------------------------------------------
 */

import { join } from 'node:path';
import { app, BrowserWindow, ipcMain } from 'electron';
import handledSquirrelEvent from 'electron-squirrel-startup';
import { CORE_SCHEMAS } from '../../contracts/core-gateway/generated/core-api';
import { STREAM_PORTS_WINDOW_MESSAGE } from '../../contracts/stream-ports/stream-messages';
import { STREAM_CHANNELS } from '../../contracts/stream-ports/stream-ports';
import { forwardCoreToWindow, registerCoreIpc, type CoreRequestHandling } from '../../entry/ipc/core/core-ipc';
import { loadAndRemember } from '../../entry/ipc/cartridges/load-cartridge';
import { registerMainRequestIpc, type MainRequestHandler } from '../../entry/ipc/core/main-request-ipc';
import { forwardWindowState, registerShellIpc } from '../../entry/ipc/shell/shell-ipc';
import { registerStreamsIpc } from '../../entry/ipc/streams/streams-ipc';
import { startThemeSync } from '../../entry/preferences/theme-sync';
import { createSenderCheck } from '../../entry/ipc/sender-check';
import { ProcessCoreGateway } from '../../infra/core-client/process-core-gateway';
import { FileRecentCartridgesService } from '../../infra/recent-cartridges/file-recent-cartridges-service';
import { FileShellStateService } from '../../infra/shell-state/file-shell-state-service';
import { StreamPortHub } from '../../infra/stream-ports/stream-port-hub';
import { startElectronCoreProcess } from '../../platform/core-process/core-process';
import { createOpenFileDialog } from '../../platform/dialog/open-file-dialog';
import { cartridgePathFromArgv, debugOptionsFromArgv } from '../../platform/launch/launch-args';
import { FileErrorLogger } from '../../platform/logging/file-error-logger';
import { JsonFileStore } from '../../platform/shell-state/json-file-store';
import { createElectronChannelFactory, createWebContentsStreamTarget } from '../../platform/stream-ports/electron-stream-ports';
import { applyThemeSource, createAppWindow, denyAllPermissions, followSystemTheme, isAppUrl, removeDefaultMenu } from '../../platform/window/app-window';
import { createWindowController } from '../../platform/window/window-controller';
import { watchWindow } from '../../platform/window/window-events';
import { MAIN_MODULES } from './module-list';

/** 应用标识，与 forge.config.mts 一致。 */
const APP_ID = 'cn.3578.ember';
/** 产品名。 */
const PRODUCT_NAME = 'Ember';
/** 原生模块文件名。 */
const NATIVE_FILE = 'ember_core.node';
/** 预装插件的目录名：开发时在 build 下，正式包在 resources 下。 */
const PLUGINS_DIRECTORY = 'plugins';
/** 主进程错误的错误码。 */
const MAIN_ERROR_CODE = 'shell.mainError';
/** 显示窗口前最多等保存的主题多久（毫秒），核心起不来时不能一直不显示。 */
const THEME_WAIT_MS = 2000;

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
if (handledSquirrelEvent || !app.requestSingleInstanceLock()) {
  app.quit();
} else {
  if (process.platform === 'win32') app.setAppUserModelId(APP_ID);

  const userDataPath = app.getPath('userData');
  const logger = new FileErrorLogger(join(userDataPath, 'logs'));
  process.on('uncaughtException', (error) => logError(logger, error, { source: 'uncaughtException' }));
  process.on('unhandledRejection', (reason) => logError(logger, reason, { source: 'unhandledRejection' }));

  const devServerUrl = MAIN_WINDOW_VITE_DEV_SERVER_URL;
  const rendererFile = join(__dirname, `../renderer/${MAIN_WINDOW_VITE_NAME}/index.html`);
  const nativePath = app.isPackaged ? join(process.resourcesPath, NATIVE_FILE) : join(app.getAppPath(), 'core', 'build', 'Release', NATIVE_FILE);
  const bundledPluginsPath = app.isPackaged ? join(process.resourcesPath, PLUGINS_DIRECTORY) : join(app.getAppPath(), 'build', PLUGINS_DIRECTORY);
  // --debug-port 指定时本次运行强制开启调试服务（不写入首选项），--debug-token 指定令牌。
  const debugLaunch = debugOptionsFromArgv(process.argv);

  const gateway = new ProcessCoreGateway({
    startProcess: () => startElectronCoreProcess(join(__dirname, 'core-host.cjs')),
    nativePath,
    userDataPath,
    appVersion: app.getVersion(),
    bundledPluginsPath,
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

  let mainWindow: BrowserWindow | undefined;
  let quitting = false;
  // 首选项里的主题由核心保存，读到后设为系统的主题来源；显示窗口前等它（最多 2 秒），避免先闪一下错误的颜色。
  const themeSync = startThemeSync({ gateway, applyThemeSource, onError: (error) => logError(logger, error, { source: 'themeSync' }) });
  const themeReady = Promise.race([themeSync.first, new Promise<void>((resolve) => setTimeout(resolve, THEME_WAIT_MS))]);
  const windowController = createWindowController(() => mainWindow, () => app.quit());
  const isTrustedSender = createSenderCheck((url) => isAppUrl(url, { rendererFile, ...(devServerUrl ? { devServerUrl } : {}) }));
  const modules = MAIN_MODULES.map((module) => module.schema);
  /** 以主进程的身份向核心发起请求所需的依赖。 */
  const coreHandling: CoreRequestHandling = { gateway, modules, schemas: CORE_SCHEMAS };

  const createMainWindow = (): void => {
    const window = createAppWindow({
      preloadPath: join(__dirname, 'preload.cjs'),
      rendererFile,
      title: PRODUCT_NAME,
      devTools: !app.isPackaged,
      placement: shellState.get().window,
      showAfter: themeReady,
      ...(devServerUrl ? { devServerUrl } : {})
    });
    mainWindow = window;
    const stopForwarding = forwardCoreToWindow(gateway, window.webContents);
    const stopFollowingTheme = followSystemTheme(window);
    watchWindow(window, {
      onPlacement: (placement) => shellState.updateWindow(placement),
      onState: forwardWindowState(window.webContents, () => windowController.state())
    });
    window.webContents.on('render-process-gone', (_event, details) => logError(logger, new Error(`渲染进程退出：${details.reason}`), { exitCode: details.exitCode }));
    window.on('closed', () => {
      stopForwarding();
      stopFollowingTheme();
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
    themeSync.stop();
    streamHub.stop();
    void gateway
      .stop()
      .catch((error: unknown) => logError(logger, error, { source: 'stopCore' }))
      .then(() => logger.flush())
      .finally(() => app.quit());
  });

  void app.whenReady().then(() => {
    denyAllPermissions();
    removeDefaultMenu();
    streamHub.start();
    gateway.start();
    registerCoreIpc({ ipc: ipcMain, gateway, modules, schemas: CORE_SCHEMAS, isTrustedSender });
    registerShellIpc({ ipc: ipcMain, isTrustedSender, state: shellState, window: windowController });

    const fileDialog = createOpenFileDialog(() => mainWindow);
    const handlers: Record<string, MainRequestHandler> = {};
    for (const module of MAIN_MODULES) Object.assign(handlers, module.createMainHandlers?.({ fileDialog, core: coreHandling, recentCartridges }));
    registerMainRequestIpc({ ipc: ipcMain, handlers, modules, schemas: CORE_SCHEMAS, isTrustedSender, onError: (error, name) => logError(logger, error, { source: 'mainRequest', name }) });
    registerStreamsIpc({
      ipc: ipcMain,
      connector: streamHub,
      isTrustedSender,
      isStreamWindow: (contents) => mainWindow?.webContents === contents,
      createTarget: (contents) => createWebContentsStreamTarget(contents, STREAM_CHANNELS.ports, STREAM_PORTS_WINDOW_MESSAGE)
    });
    createMainWindow();

    const cartridgePath = cartridgePathFromArgv(process.argv);
    if (cartridgePath !== undefined) {
      loadAndRemember({ core: coreHandling, recent: recentCartridges }, cartridgePath).catch((error: unknown) => logError(logger, error, { source: 'launchCartridge' }));
    }
  });
}
