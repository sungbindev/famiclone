/*
 * ------------------------------------------------------------------------
 * 名称：main.ts
 * 说明：主进程组装根：Windows 安装事件、单实例、错误日志、启动核心进程、登记核心请求的 IPC、创建主窗口，退出时停止核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只创建实现并注入，不写业务判断；原生模块在开发时取 core/build/Release，正式包取应用的 resources 目录。
 * ------------------------------------------------------------------------
 */

import { join } from 'node:path';
import { app, BrowserWindow, ipcMain } from 'electron';
import handledSquirrelEvent from 'electron-squirrel-startup';
import { CORE_SCHEMAS } from '../../contracts/core-gateway/generated/core-api';
import { forwardCoreToWindow, registerCoreIpc } from '../../entry/ipc/core/core-ipc';
import { createSenderCheck } from '../../entry/ipc/sender-check';
import { ProcessCoreGateway } from '../../infra/core-client/process-core-gateway';
import { startElectronCoreProcess } from '../../platform/core-process/core-process';
import { FileErrorLogger } from '../../platform/logging/file-error-logger';
import { createAppWindow, denyAllPermissions, isAppUrl, removeDefaultMenu } from '../../platform/window/app-window';
import { MAIN_MODULES } from './module-list';

/** 应用标识，与 forge.config.mts 一致。 */
const APP_ID = 'cn.3578.ember';
/** 产品名。 */
const PRODUCT_NAME = 'Ember';
/** 原生模块文件名。 */
const NATIVE_FILE = 'ember_core.node';
/** 主进程错误的错误码。 */
const MAIN_ERROR_CODE = 'shell.mainError';

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

  const gateway = new ProcessCoreGateway({
    startProcess: () => startElectronCoreProcess(join(__dirname, 'core-host.cjs')),
    nativePath,
    userDataPath,
    appVersion: app.getVersion(),
    logger
  });

  let mainWindow: BrowserWindow | undefined;
  let quitting = false;
  const isTrustedSender = createSenderCheck((url) => isAppUrl(url, { rendererFile, ...(devServerUrl ? { devServerUrl } : {}) }));

  const createMainWindow = (): void => {
    const window = createAppWindow({
      preloadPath: join(__dirname, 'preload.cjs'),
      rendererFile,
      title: PRODUCT_NAME,
      devTools: !app.isPackaged,
      ...(devServerUrl ? { devServerUrl } : {})
    });
    mainWindow = window;
    const stopForwarding = forwardCoreToWindow(gateway, window.webContents);
    window.webContents.on('render-process-gone', (_event, details) => logError(logger, new Error(`渲染进程退出：${details.reason}`), { exitCode: details.exitCode }));
    window.on('closed', () => {
      stopForwarding();
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
    void gateway
      .stop()
      .catch((error: unknown) => logError(logger, error, { source: 'stopCore' }))
      .then(() => logger.flush())
      .finally(() => app.quit());
  });

  void app.whenReady().then(() => {
    denyAllPermissions();
    removeDefaultMenu();
    gateway.start();
    registerCoreIpc({ ipc: ipcMain, gateway, modules: MAIN_MODULES.map((module) => module.schema), schemas: CORE_SCHEMAS, isTrustedSender });
    createMainWindow();
  });
}
